#pragma once

#include "Flawy/Renderer/Shader/ShaderStage.hpp"

#include <slang.h>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace Flawy::Renderer
{
    struct BgfxBindingRules
    {
        uint32_t uUniformBlockBinding = 0;
        uint32_t uTextureBindingBase = 0;
        uint32_t uSamplerBindingOffset = 0;
        SlangParameterCategory eUniformBlockCategory = SLANG_PARAMETER_CATEGORY_NONE;
        bool bReflectionReportsUniformBlockBinding = true;
    };

    class IShaderBackendEmitter
    {
    public:
        virtual ~IShaderBackendEmitter() = default;

        [[nodiscard]] virtual SlangCompileTarget GetTargetFormat() const noexcept = 0;
        [[nodiscard]] virtual const char* GetProfileName() const noexcept = 0;
        virtual void AppendTargetOptions(ShaderStage eStage, std::vector<slang::CompilerOptionEntry>& vOptions) const = 0;
        [[nodiscard]] virtual BgfxBindingRules GetBindingRules(ShaderStage eStage) const noexcept = 0;
        [[nodiscard]] virtual std::vector<std::byte> EmitShaderCode(std::span<const std::byte> spanTargetCode, ShaderStage eStage) const = 0;
    };
}
