#pragma once

#include "Flawy/Animation/JointTransform.hpp"

#include <ufbx.h>

#include <glm/ext/quaternion_float.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include <string>

namespace Flawy::Import
{
    class UfbxConversion final
    {
    public:
        UfbxConversion() = delete;

        [[nodiscard]] static std::string ToString(const ufbx_string& oText);
        [[nodiscard]] static glm::vec3 ToVector(const ufbx_vec3& vecValue) noexcept;
        [[nodiscard]] static glm::quat ToQuaternion(const ufbx_quat& quatValue) noexcept;
        [[nodiscard]] static glm::mat4 ToMatrix(const ufbx_matrix& matValue) noexcept;
        [[nodiscard]] static Animation::JointTransform ToJointTransform(const ufbx_transform& oTransform) noexcept;
        [[nodiscard]] static std::string FormatError(const ufbx_error& oError);
    };
}
