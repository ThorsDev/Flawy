#include "Animation/Ozz/OzzMathConversion.hpp"

#include "Animation/AnimationValueChecks.hpp"

namespace Flawy::Animation
{
    ozz::math::Float3 OzzMathConversion::ToOzz(const glm::vec3& vecValue) noexcept
    {
        return {vecValue.x, vecValue.y, vecValue.z};
    }

    ozz::math::Quaternion OzzMathConversion::ToOzz(const glm::quat& quatValue) noexcept
    {
        const glm::quat quatNormalized = AnimationValueChecks::NormalizeRotation(quatValue);
        return {quatNormalized.x, quatNormalized.y, quatNormalized.z, quatNormalized.w};
    }

    ozz::math::Transform OzzMathConversion::ToOzz(const JointTransform& oTransform) noexcept
    {
        ozz::math::Transform oResult;
        oResult.translation = ToOzz(oTransform.vecTranslation);
        oResult.rotation = ToOzz(oTransform.quatRotation);
        oResult.scale = ToOzz(oTransform.vecScale);
        return oResult;
    }

    glm::mat4 OzzMathConversion::ToGlm(const ozz::math::Float4x4& oMatrix) noexcept
    {
        glm::mat4 matResult(1.0f);
        for (glm::length_t iColumn = 0; iColumn < 4; ++iColumn)
        {
            ozz::math::StorePtrU(oMatrix.cols[iColumn], &matResult[iColumn].x);
        }
        return matResult;
    }
}
