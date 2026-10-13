#pragma once

#include "Flawy/Renderer/RendererBackend.hpp"
#include "Flawy/Renderer/Shader/CompiledShader.hpp"
#include "Flawy/Renderer/Shader/ShaderSource.hpp"
#include "Flawy/Renderer/Shader/ShaderStage.hpp"

#include <memory>
#include <string_view>

namespace Flawy::Renderer
{
    class ShaderCompiler final
    {
    public:
        explicit ShaderCompiler(RendererBackend eBackend);
        ~ShaderCompiler();

        ShaderCompiler(const ShaderCompiler&) = delete;
        ShaderCompiler& operator=(const ShaderCompiler&) = delete;
        ShaderCompiler(ShaderCompiler&&) = delete;
        ShaderCompiler& operator=(ShaderCompiler&&) = delete;

        [[nodiscard]] CompiledShader Compile(const ShaderSource& oSource, ShaderStage eStage, std::string_view svEntryPoint);

    private:
        struct Impl;
        std::unique_ptr<Impl> m_upImpl;
    };
}
