#include "Import/UfbxConversion.hpp"

#include <glm/vec4.hpp>

#include <algorithm>
#include <array>
#include <cstddef>

namespace Flawy::Import
{
    namespace
    {
        constexpr size_t kErrorMessageCapacity = 2048;
    }

    std::string UfbxConversion::ToString(const ufbx_string& oText)
    {
        return std::string(oText.data, oText.length);
    }

    glm::vec3 UfbxConversion::ToVector(const ufbx_vec3& vecValue) noexcept
    {
        return {static_cast<float>(vecValue.x), static_cast<float>(vecValue.y), static_cast<float>(vecValue.z)};
    }

    glm::quat UfbxConversion::ToQuaternion(const ufbx_quat& quatValue) noexcept
    {
        return glm::quat(static_cast<float>(quatValue.w), static_cast<float>(quatValue.x), static_cast<float>(quatValue.y), static_cast<float>(quatValue.z));
    }

    glm::mat4 UfbxConversion::ToMatrix(const ufbx_matrix& matValue) noexcept
    {
        return glm::mat4(glm::vec4(ToVector(matValue.cols[0]), 0.0f),
                         glm::vec4(ToVector(matValue.cols[1]), 0.0f),
                         glm::vec4(ToVector(matValue.cols[2]), 0.0f),
                         glm::vec4(ToVector(matValue.cols[3]), 1.0f));
    }

    Animation::JointTransform UfbxConversion::ToJointTransform(const ufbx_transform& oTransform) noexcept
    {
        Animation::JointTransform oResult;
        oResult.vecTranslation = ToVector(oTransform.translation);
        oResult.quatRotation = ToQuaternion(oTransform.rotation);
        oResult.vecScale = ToVector(oTransform.scale);
        return oResult;
    }

    std::string UfbxConversion::FormatError(const ufbx_error& oError)
    {
        std::array<char, kErrorMessageCapacity> aBuffer{};
        const size_t uLength = ufbx_format_error(aBuffer.data(), aBuffer.size(), &oError);
        return std::string(aBuffer.data(), std::min(uLength, aBuffer.size()));
    }
}
