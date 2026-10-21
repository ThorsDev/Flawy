#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace Flawy::Renderer
{
    struct ViewParameters
    {
        glm::mat4 matView{1.0f};
        glm::mat4 matProjection{1.0f};
        glm::vec3 vecCameraPosition{0.0f};
        float fTimeSeconds = 0.0f;
    };
}
