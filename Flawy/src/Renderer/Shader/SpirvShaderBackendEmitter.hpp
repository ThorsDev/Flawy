#pragma once

#include "Renderer/Shader/IShaderBackendEmitter.hpp"

namespace Flawy::Renderer
{
    class SpirvShaderBackendEmitter final : public IShaderBackendEmitter
    {
    public:
        [[nodiscard]] SlangCompileTarget GetTargetFormat() const noexcept override;
        [[nodiscard]] const char* GetProfileName() const noexcept override;
        void AppendTargetOptions(ShaderStage eStage, std::vector<slang::CompilerOptionEntry>& vOptions) const override;
        [[nodiscard]] BgfxBindingRules GetBindingRules(ShaderStage eStage) const noexcept override;
        [[nodiscard]] std::vector<std::byte> EmitShaderCode(std::span<const std::byte> spanTargetCode, ShaderStage eStage) const override;
    };
}
