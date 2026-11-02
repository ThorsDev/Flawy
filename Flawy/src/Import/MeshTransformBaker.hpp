#pragma once

#include "Flawy/Assets/MeshData.hpp"

#include <glm/mat4x4.hpp>

#include <cstddef>

namespace Flawy::Import
{
    class MeshTransformBaker final
    {
    public:
        MeshTransformBaker() = delete;

        static void Apply(Assets::MeshData& oMesh, const glm::mat4& matTransform, size_t uFirstVertex, size_t uFirstIndex);
    };
}
