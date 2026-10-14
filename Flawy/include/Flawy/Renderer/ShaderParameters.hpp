#pragma once

#include "Flawy/Renderer/SamplerState.hpp"

#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

namespace Flawy::Renderer
{
    class ShaderProgram;
    class Texture;

    struct ShaderParameterId
    {
        uint32_t uIndex = 0xffffffffu;

        [[nodiscard]] bool IsValid() const noexcept;
    };

    class ShaderParameters final
    {
    public:
        struct Impl;

        explicit ShaderParameters(const ShaderProgram& oProgram);
        ~ShaderParameters();

        ShaderParameters(const ShaderParameters&) = delete;
        ShaderParameters& operator=(const ShaderParameters&) = delete;
        ShaderParameters(ShaderParameters&&) = delete;
        ShaderParameters& operator=(ShaderParameters&&) = delete;

        [[nodiscard]] ShaderParameterId Find(std::string_view svName) const noexcept;
        [[nodiscard]] ShaderParameterId Require(std::string_view svName) const;

        void SetVec4(ShaderParameterId oId, const glm::vec4& vecValue);
        void SetVec4Array(ShaderParameterId oId, std::span<const glm::vec4> spanValues);
        void SetMat3(ShaderParameterId oId, const glm::mat3& matValue);
        void SetMat3Array(ShaderParameterId oId, std::span<const glm::mat3> spanValues);
        void SetMat4(ShaderParameterId oId, const glm::mat4& matValue);
        void SetMat4Array(ShaderParameterId oId, std::span<const glm::mat4> spanValues);
        void SetTexture(ShaderParameterId oId, const Texture& oTexture);
        void SetTexture(ShaderParameterId oId, const Texture& oTexture, const SamplerState& oSampler);

        [[nodiscard]] const ShaderProgram& GetProgram() const noexcept;
        [[nodiscard]] const Impl& GetImpl() const noexcept;

    private:
        std::unique_ptr<Impl> m_upImpl;
    };
}
