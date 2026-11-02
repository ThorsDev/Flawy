#include "Animation/AnimationValueChecks.hpp"

#include <glm/ext/quaternion_geometric.hpp>

#include <cmath>

namespace Flawy::Animation
{
    namespace
    {
        constexpr float kMinimumRotationLength = 1.0e-6f;
    }

    bool AnimationValueChecks::IsFinite(const glm::vec3& vecValue) noexcept
    {
        return std::isfinite(vecValue.x) && std::isfinite(vecValue.y) && std::isfinite(vecValue.z);
    }

    bool AnimationValueChecks::IsFinite(const glm::quat& quatValue) noexcept
    {
        return std::isfinite(quatValue.x) && std::isfinite(quatValue.y) && std::isfinite(quatValue.z) && std::isfinite(quatValue.w);
    }

    bool AnimationValueChecks::IsFinite(const glm::mat4& matValue) noexcept
    {
        for (glm::length_t iColumn = 0; iColumn < 4; ++iColumn)
        {
            for (glm::length_t iRow = 0; iRow < 4; ++iRow)
            {
                if (!std::isfinite(matValue[iColumn][iRow]))
                {
                    return false;
                }
            }
        }
        return true;
    }

    bool AnimationValueChecks::IsUsableRotation(const glm::quat& quatValue) noexcept
    {
        return IsFinite(quatValue) && glm::length(quatValue) > kMinimumRotationLength;
    }

    glm::quat AnimationValueChecks::NormalizeRotation(const glm::quat& quatValue) noexcept
    {
        return glm::normalize(quatValue);
    }
}
