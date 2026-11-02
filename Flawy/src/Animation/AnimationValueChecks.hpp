#pragma once

#include <glm/ext/quaternion_float.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace Flawy::Animation
{
    class AnimationValueChecks final
    {
    public:
        AnimationValueChecks() = delete;

        [[nodiscard]] static bool IsFinite(const glm::vec3& vecValue) noexcept;
        [[nodiscard]] static bool IsFinite(const glm::quat& quatValue) noexcept;
        [[nodiscard]] static bool IsFinite(const glm::mat4& matValue) noexcept;
        [[nodiscard]] static bool IsUsableRotation(const glm::quat& quatValue) noexcept;
        [[nodiscard]] static glm::quat NormalizeRotation(const glm::quat& quatValue) noexcept;
    };
}
