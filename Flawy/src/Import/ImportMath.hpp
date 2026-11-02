#pragma once

#include "Flawy/Animation/JointTransform.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <cstdint>

namespace Flawy::Import
{
    class ImportMath final
    {
    public:
        ImportMath() = delete;

        [[nodiscard]] static glm::mat4 ComposeTransform(const Animation::JointTransform& oTransform);
        [[nodiscard]] static Animation::JointTransform DecomposeTransform(const glm::mat4& matTransform);
        [[nodiscard]] static bool HasUniformScale(const glm::vec3& vecScale) noexcept;
        [[nodiscard]] static uint32_t PackColorAbgr(const glm::vec4& vecColor) noexcept;
    };
}
