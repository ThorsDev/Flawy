#pragma once

#include "Flawy/Renderer/Shader/CompiledShader.hpp"

#include <memory>

namespace Flawy::Renderer
{
    class ShaderProgram final
    {
    public:
        struct Impl;

        ShaderProgram(const CompiledShader& oVertexShader, const CompiledShader& oFragmentShader);
        ~ShaderProgram();

        ShaderProgram(const ShaderProgram&) = delete;
        ShaderProgram& operator=(const ShaderProgram&) = delete;
        ShaderProgram(ShaderProgram&&) = delete;
        ShaderProgram& operator=(ShaderProgram&&) = delete;

        [[nodiscard]] const Impl& GetImpl() const noexcept;

    private:
        std::unique_ptr<Impl> m_upImpl;
    };
}
