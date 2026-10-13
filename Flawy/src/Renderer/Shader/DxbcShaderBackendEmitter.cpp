#include "Renderer/Shader/DxbcShaderBackendEmitter.hpp"

namespace Flawy::Renderer
{
    SlangCompileTarget DxbcShaderBackendEmitter::GetTargetFormat() const noexcept
    {
        return SLANG_DXBC;
    }

    const char* DxbcShaderBackendEmitter::GetProfileName() const noexcept
    {
        return "sm_5_0";
    }

    void DxbcShaderBackendEmitter::AppendTargetOptions(ShaderStage, std::vector<slang::CompilerOptionEntry>&) const
    {
    }

    BgfxBindingRules DxbcShaderBackendEmitter::GetBindingRules(ShaderStage) const noexcept
    {
        return {0, 0, 0, SLANG_PARAMETER_CATEGORY_CONSTANT_BUFFER, true};
    }

    std::vector<std::byte> DxbcShaderBackendEmitter::EmitShaderCode(std::span<const std::byte> spanTargetCode, ShaderStage) const
    {
        return {spanTargetCode.begin(), spanTargetCode.end()};
    }
}
