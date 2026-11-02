#include "Import/NormalGenerator.hpp"

#include <glm/geometric.hpp>

#include <cstdint>

namespace Flawy::Import
{
    namespace
    {
        constexpr float kMinimumLengthSquared = 1.0e-12f;
        constexpr glm::vec3 kFallbackNormal = {0.0f, 1.0f, 0.0f};
    }

    void NormalGenerator::Generate(Assets::MeshData& oMesh, size_t uFirstVertex, size_t uFirstIndex)
    {
        const std::vector<glm::vec3> vSums = AccumulateFaceNormals(oMesh, uFirstIndex);
        oMesh.vNormals.resize(oMesh.vPositions.size(), kFallbackNormal);
        for (size_t uVertex = uFirstVertex; uVertex < oMesh.vNormals.size(); ++uVertex)
        {
            oMesh.vNormals[uVertex] = NormalizeOrFallback(vSums[uVertex]);
        }
    }

    void NormalGenerator::GenerateMissing(Assets::MeshData& oMesh, const std::vector<bool>& vHasNormal)
    {
        const std::vector<glm::vec3> vSums = AccumulateFaceNormals(oMesh, 0);
        oMesh.vNormals.resize(oMesh.vPositions.size(), kFallbackNormal);
        for (size_t uVertex = 0; uVertex < oMesh.vNormals.size(); ++uVertex)
        {
            if (uVertex >= vHasNormal.size() || !vHasNormal[uVertex])
            {
                oMesh.vNormals[uVertex] = NormalizeOrFallback(vSums[uVertex]);
            }
        }
    }

    std::vector<glm::vec3> NormalGenerator::AccumulateFaceNormals(const Assets::MeshData& oMesh, size_t uFirstIndex)
    {
        std::vector<glm::vec3> vSums(oMesh.vPositions.size(), glm::vec3(0.0f));
        for (size_t uIndex = uFirstIndex; uIndex + 2 < oMesh.vIndices.size(); uIndex += 3)
        {
            const uint32_t uA = oMesh.vIndices[uIndex];
            const uint32_t uB = oMesh.vIndices[uIndex + 1];
            const uint32_t uC = oMesh.vIndices[uIndex + 2];
            if (uA >= vSums.size() || uB >= vSums.size() || uC >= vSums.size())
            {
                continue;
            }
            const glm::vec3 vecFace = glm::cross(oMesh.vPositions[uB] - oMesh.vPositions[uA], oMesh.vPositions[uC] - oMesh.vPositions[uA]);
            vSums[uA] += vecFace;
            vSums[uB] += vecFace;
            vSums[uC] += vecFace;
        }
        return vSums;
    }

    glm::vec3 NormalGenerator::NormalizeOrFallback(const glm::vec3& vecSum) noexcept
    {
        const float fLengthSquared = glm::dot(vecSum, vecSum);
        return fLengthSquared > kMinimumLengthSquared ? vecSum / glm::sqrt(fLengthSquared) : kFallbackNormal;
    }
}
