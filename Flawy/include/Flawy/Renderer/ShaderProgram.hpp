#pragma once

#include "Flawy/Renderer/Shader/CompiledShader.hpp"
#include "Flawy/Renderer/Shader/ShaderInterface.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

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

        [[nodiscard]] std::span<const ShaderUniform> GetUniforms() const noexcept;
        [[nodiscard]] std::optional<uint32_t> FindUniform(std::string_view svName) const noexcept;
        [[nodiscard]] const Impl& GetImpl() const noexcept;

    private:
        std::unique_ptr<Impl> m_upImpl;
    };
}
