#include "Renderer/Shader/ShaderSourceFileSystem.hpp"

#include "Flawy/Renderer/Shader/ShaderSource.hpp"

#include <algorithm>
#include <cstring>
#include <optional>

namespace Flawy::Renderer
{
    namespace
    {
        constexpr std::string_view kCurrentDirectoryPrefix = "./";

        bool IsSameUuid(const SlangUUID& oLeft, const SlangUUID& oRight) noexcept
        {
            return std::memcmp(&oLeft, &oRight, sizeof(SlangUUID)) == 0;
        }
    }

    ShaderSourceFileSystem::ShaderSourceFileSystem(const IShaderSourceProvider& oProvider) noexcept
        : m_oProvider(oProvider)
    {
    }

    SlangResult ShaderSourceFileSystem::queryInterface(const SlangUUID& oUuid, void** pOutObject)
    {
        if (pOutObject == nullptr)
        {
            return SLANG_E_INVALID_ARG;
        }

        void* pInterface = FindInterface(oUuid);
        if (pInterface == nullptr)
        {
            *pOutObject = nullptr;
            return SLANG_E_NO_INTERFACE;
        }

        addRef();
        *pOutObject = pInterface;
        return SLANG_OK;
    }

    uint32_t ShaderSourceFileSystem::addRef()
    {
        return ++m_uReferenceCount;
    }

    uint32_t ShaderSourceFileSystem::release()
    {
        return m_uReferenceCount > 1 ? --m_uReferenceCount : m_uReferenceCount;
    }

    void* ShaderSourceFileSystem::castAs(const SlangUUID& oUuid)
    {
        return FindInterface(oUuid);
    }

    SlangResult ShaderSourceFileSystem::loadFile(const char* szPath, ISlangBlob** pOutBlob)
    {
        *pOutBlob = nullptr;
        if (szPath == nullptr)
        {
            return SLANG_E_INVALID_ARG;
        }

        const std::optional<ShaderSource> optSource = m_oProvider.Load(NormalizePath(szPath));
        if (!optSource)
        {
            return SLANG_E_NOT_FOUND;
        }

        *pOutBlob = slang_createBlob(optSource->sCode.data(), optSource->sCode.size());
        return *pOutBlob != nullptr ? SLANG_OK : SLANG_E_OUT_OF_MEMORY;
    }

    std::string ShaderSourceFileSystem::NormalizePath(std::string_view svPath)
    {
        std::string sPath(svPath);
        std::ranges::replace(sPath, '\\', '/');
        while (std::string_view(sPath).starts_with(kCurrentDirectoryPrefix))
        {
            sPath.erase(0, kCurrentDirectoryPrefix.size());
        }
        return sPath;
    }

    void* ShaderSourceFileSystem::FindInterface(const SlangUUID& oUuid) noexcept
    {
        if (IsSameUuid(oUuid, ISlangUnknown::getTypeGuid()) || IsSameUuid(oUuid, ISlangCastable::getTypeGuid()) || IsSameUuid(oUuid, ISlangFileSystem::getTypeGuid()))
        {
            return static_cast<ISlangFileSystem*>(this);
        }
        return nullptr;
    }
}
