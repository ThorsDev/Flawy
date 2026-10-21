#pragma once

#include <glm/vec3.hpp>

#include <cstddef>
#include <optional>
#include <vector>

namespace Flawy::Renderer
{
    struct DirectionalLight
    {
        glm::vec3 vecDirection{0.0f, -1.0f, 0.0f};
        glm::vec3 vecColor{1.0f};
        float fIntensity = 1.0f;
    };

    struct PointLight
    {
        glm::vec3 vecPosition{0.0f};
        float fRadius = 10.0f;
        glm::vec3 vecColor{1.0f};
        float fIntensity = 1.0f;
    };

    struct LightEnvironment
    {
        static constexpr size_t kMaxPointLights = 4;

        std::optional<DirectionalLight> optDirectional;
        std::vector<PointLight> vPointLights;
        glm::vec3 vecSkyAmbient{0.05f};
        glm::vec3 vecGroundAmbient{0.02f};
    };
}
