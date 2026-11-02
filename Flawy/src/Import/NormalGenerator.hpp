#pragma once

#include "Flawy/Assets/MeshData.hpp"

#include <glm/vec3.hpp>

#include <cstddef>
#include <vector>

namespace Flawy::Import
{
    class NormalGenerator final
    {
    public:
        NormalGenerator() = delete;

        static void Generate(Assets::MeshData& oMesh, size_t uFirstVertex, size_t uFirstIndex);
        static void GenerateMissing(Assets::MeshData& oMesh, const std::vector<bool>& vHasNormal);

    private:
        [[nodiscard]] static std::vector<glm::vec3> AccumulateFaceNormals(const Assets::MeshData& oMesh, size_t uFirstIndex);
        [[nodiscard]] static glm::vec3 NormalizeOrFallback(const glm::vec3& vecSum) noexcept;
    };
}
