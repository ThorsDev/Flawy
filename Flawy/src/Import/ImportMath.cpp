#include "Import/ImportMath.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/geometric.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>

#include <algorithm>
#include <cmath>

namespace Flawy::Import
{
    namespace
    {
        constexpr float kMinimumScale = 1.0e-8f;
        constexpr float kUniformScaleTolerance = 1.0e-4f;
        constexpr float kColorChannelMaximum = 255.0f;

        uint32_t PackChannel(float fValue) noexcept
        {
            const float fClamped = std::isfinite(fValue) ? std::clamp(fValue, 0.0f, 1.0f) : 0.0f;
            return static_cast<uint32_t>(std::lround(fClamped * kColorChannelMaximum));
        }
    }

    glm::mat4 ImportMath::ComposeTransform(const Animation::JointTransform& oTransform)
    {
        const glm::mat4 matTranslation = glm::translate(glm::mat4(1.0f), oTransform.vecTranslation);
        const glm::mat4 matRotation = glm::mat4_cast(oTransform.quatRotation);
        return glm::scale(matTranslation * matRotation, oTransform.vecScale);
    }

    Animation::JointTransform ImportMath::DecomposeTransform(const glm::mat4& matTransform)
    {
        Animation::JointTransform oTransform;
        oTransform.vecTranslation = glm::vec3(matTransform[3]);

        glm::mat3 matBasis(matTransform);
        glm::vec3 vecScale(glm::length(matBasis[0]), glm::length(matBasis[1]), glm::length(matBasis[2]));
        if (glm::determinant(matBasis) < 0.0f)
        {
            vecScale.x = -vecScale.x;
        }
        oTransform.vecScale = vecScale;

        if (std::abs(vecScale.x) < kMinimumScale || std::abs(vecScale.y) < kMinimumScale || std::abs(vecScale.z) < kMinimumScale)
        {
            return oTransform;
        }

        matBasis[0] /= vecScale.x;
        matBasis[1] /= vecScale.y;
        matBasis[2] /= vecScale.z;
        oTransform.quatRotation = glm::normalize(glm::quat_cast(matBasis));
        return oTransform;
    }

    bool ImportMath::HasUniformScale(const glm::vec3& vecScale) noexcept
    {
        const float fLargest = std::max({std::abs(vecScale.x), std::abs(vecScale.y), std::abs(vecScale.z), 1.0f});
        return std::abs(vecScale.x - vecScale.y) <= kUniformScaleTolerance * fLargest &&
               std::abs(vecScale.x - vecScale.z) <= kUniformScaleTolerance * fLargest;
    }

    uint32_t ImportMath::PackColorAbgr(const glm::vec4& vecColor) noexcept
    {
        return (PackChannel(vecColor.a) << 24u) | (PackChannel(vecColor.b) << 16u) | (PackChannel(vecColor.g) << 8u) | PackChannel(vecColor.r);
    }
}
