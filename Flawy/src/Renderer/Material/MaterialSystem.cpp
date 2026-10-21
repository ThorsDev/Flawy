#include "Flawy/Renderer/Material/MaterialSystem.hpp"

#include "Flawy/Core/Logger.hpp"
#include "Flawy/Renderer/RenderDevice.hpp"
#include "Flawy/Renderer/Shader/ShaderCache.hpp"
#include "Flawy/Renderer/Shader/ShaderCompiler.hpp"
#include "Flawy/Renderer/Texture.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <span>
#include <string_view>

namespace Flawy::Renderer
{
    namespace
    {
        constexpr uint32_t kWhiteRgba = 0xffffffffu;
        constexpr uint32_t kFlatNormalRgba = 0xffff8080u;

        TextureDescriptor BuildSolidDescriptor(std::string_view svName)
        {
            TextureDescriptor oDescriptor;
            oDescriptor.uWidth = 1;
            oDescriptor.uHeight = 1;
            oDescriptor.eFormat = TextureFormat::Rgba8;
            oDescriptor.oSampler.eFilter = TextureFilter::Point;
            oDescriptor.sDebugName = std::string(svName);
            return oDescriptor;
        }

        std::span<const std::byte> AsPixelBytes(const uint32_t& uPixel) noexcept
        {
            return std::as_bytes(std::span<const uint32_t>(&uPixel, 1));
        }
    }

    struct MaterialSystem::Impl
    {
        Impl(RendererBackend eRendererBackend, const MaterialSystemSpecification& oSpecification)
            : eBackend(eRendererBackend)
            , oCompiler(eRendererBackend)
            , oShaderCache(oSpecification.optShaderCacheDirectory)
            , oWhite(BuildSolidDescriptor("MaterialWhite"), AsPixelBytes(kWhiteRgba))
            , oFlatNormal(BuildSolidDescriptor("MaterialFlatNormal"), AsPixelBytes(kFlatNormalRgba))
        {
        }

        RendererBackend eBackend;
        ShaderCompiler oCompiler;
        ShaderCache oShaderCache;
        Texture oWhite;
        Texture oFlatNormal;
    };

    MaterialSystem::MaterialSystem(RenderDevice& oDevice, const MaterialSystemSpecification& oSpecification)
        : m_upImpl(std::make_unique<Impl>(oDevice.GetBackend(), oSpecification))
    {
        if (oSpecification.optShaderCacheDirectory && Core::Logger::IsInitialized())
        {
            const std::u8string sDirectory = oSpecification.optShaderCacheDirectory->u8string();
            Core::Logger::Get().info("Shader cache directory: {}", reinterpret_cast<const char*>(sDirectory.c_str()));
        }
    }

    MaterialSystem::~MaterialSystem() = default;

    ShaderCompiler& MaterialSystem::GetCompiler() noexcept
    {
        return m_upImpl->oCompiler;
    }

    ShaderCache& MaterialSystem::GetShaderCache() noexcept
    {
        return m_upImpl->oShaderCache;
    }

    const Texture& MaterialSystem::GetWhiteTexture() const noexcept
    {
        return m_upImpl->oWhite;
    }

    const Texture& MaterialSystem::GetFlatNormalTexture() const noexcept
    {
        return m_upImpl->oFlatNormal;
    }

    RendererBackend MaterialSystem::GetBackend() const noexcept
    {
        return m_upImpl->eBackend;
    }
}
