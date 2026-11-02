#include "Import/MeshTransformBaker.hpp"

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <utility>

namespace Flawy::Import
{
    namespace
    {
        constexpr float kMinimumLengthSquared = 1.0e-12f;

        glm::vec3 SafeNormalize(const glm::vec3& vecValue, const glm::vec3& vecFallback) noexcept
        {
            const float fLengthSquared = glm::dot(vecValue, vecValue);
            return fLengthSquared > kMinimumLengthSquared ? vecValue / glm::sqrt(fLengthSquared) : vecFallback;
        }
    }

    void MeshTransformBaker::Apply(Assets::MeshData& oMesh, const glm::mat4& matTransform, size_t uFirstVertex, size_t uFirstIndex)
    {
        const glm::mat3 matBasis(matTransform);
        const glm::mat3 matNormal = glm::transpose(glm::inverse(matBasis));
        const bool bMirrored = glm::determinant(matBasis) < 0.0f;

        for (size_t uVertex = uFirstVertex; uVertex < oMesh.vPositions.size(); ++uVertex)
        {
            oMesh.vPositions[uVertex] = glm::vec3(matTransform * glm::vec4(oMesh.vPositions[uVertex], 1.0f));
        }
        for (size_t uVertex = uFirstVertex; uVertex < oMesh.vNormals.size(); ++uVertex)
        {
            oMesh.vNormals[uVertex] = SafeNormalize(matNormal * oMesh.vNormals[uVertex], glm::vec3(0.0f, 1.0f, 0.0f));
        }
        for (size_t uVertex = uFirstVertex; uVertex < oMesh.vTangents.size(); ++uVertex)
        {
            glm::vec4& vecTangent = oMesh.vTangents[uVertex];
            const glm::vec3 vecDirection = SafeNormalize(matBasis * glm::vec3(vecTangent), glm::vec3(1.0f, 0.0f, 0.0f));
            vecTangent = glm::vec4(vecDirection, bMirrored ? -vecTangent.w : vecTangent.w);
        }

        if (!bMirrored)
        {
            return;
        }
        for (size_t uIndex = uFirstIndex; uIndex + 2 < oMesh.vIndices.size(); uIndex += 3)
        {
            std::swap(oMesh.vIndices[uIndex + 1], oMesh.vIndices[uIndex + 2]);
        }
    }
}
