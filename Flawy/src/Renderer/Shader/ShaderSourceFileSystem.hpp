#pragma once

#include "Flawy/Renderer/Shader/IShaderSourceProvider.hpp"

#include <slang.h>

#include <cstdint>
#include <string>
#include <string_view>

namespace Flawy::Renderer
{
    class ShaderSourceFileSystem final : public ISlangFileSystem
    {
    public:
        explicit ShaderSourceFileSystem(const IShaderSourceProvider& oProvider) noexcept;
        virtual ~ShaderSourceFileSystem() = default;

        ShaderSourceFileSystem(const ShaderSourceFileSystem&) = delete;
        ShaderSourceFileSystem& operator=(const ShaderSourceFileSystem&) = delete;
        ShaderSourceFileSystem(ShaderSourceFileSystem&&) = delete;
        ShaderSourceFileSystem& operator=(ShaderSourceFileSystem&&) = delete;

        SLANG_NO_THROW SlangResult SLANG_MCALL queryInterface(const SlangUUID& oUuid, void** pOutObject) override;
        SLANG_NO_THROW uint32_t SLANG_MCALL addRef() override;
        SLANG_NO_THROW uint32_t SLANG_MCALL release() override;
        SLANG_NO_THROW void* SLANG_MCALL castAs(const SlangUUID& oUuid) override;
        SLANG_NO_THROW SlangResult SLANG_MCALL loadFile(const char* szPath, ISlangBlob** pOutBlob) override;

    private:
        [[nodiscard]] static std::string NormalizePath(std::string_view svPath);
        [[nodiscard]] void* FindInterface(const SlangUUID& oUuid) noexcept;

        const IShaderSourceProvider& m_oProvider;
        uint32_t m_uReferenceCount = 1;
    };
}
