#pragma once

#include <glm/ext/quaternion_float.hpp>
#include <glm/vec3.hpp>

namespace Flawy::Animation
{
    struct JointTransform
    {
        glm::vec3 vecTranslation{0.0f};
        glm::quat quatRotation{1.0f, 0.0f, 0.0f, 0.0f};
        glm::vec3 vecScale{1.0f};
    };
}
