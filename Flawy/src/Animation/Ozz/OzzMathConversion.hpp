#pragma once

#include "Flawy/Animation/JointTransform.hpp"

#include <glm/mat4x4.hpp>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/maths/transform.h>

namespace Flawy::Animation
{
    class OzzMathConversion final
    {
    public:
        OzzMathConversion() = delete;

        [[nodiscard]] static ozz::math::Float3 ToOzz(const glm::vec3& vecValue) noexcept;
        [[nodiscard]] static ozz::math::Quaternion ToOzz(const glm::quat& quatValue) noexcept;
        [[nodiscard]] static ozz::math::Transform ToOzz(const JointTransform& oTransform) noexcept;
        [[nodiscard]] static glm::mat4 ToGlm(const ozz::math::Float4x4& oMatrix) noexcept;
    };
}
