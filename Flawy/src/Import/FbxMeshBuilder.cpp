#include "Import/FbxMeshBuilder.hpp"

#include "Import/FbxSkinBinding.hpp"
#include "Import/ImportMath.hpp"
#include "Import/MeshTransformBaker.hpp"
#include "Import/MeshWelder.hpp"
#include "Import/UfbxConversion.hpp"

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <array>
#include <format>
#include <limits>
#include <stdexcept>
#include <utility>

namespace Flawy::Import
{
    namespace
    {
        constexpr size_t kMaxInfluences = 4;
        constexpr uint32_t kMaxSkinningJoint = std::numeric_limits<uint8_t>::max();
        constexpr uint32_t kOpaqueWhiteAbgr = 0xffffffffu;
        constexpr float kMinimumWeightSum = 1.0e-6f;
    }

    FbxMeshBuilder::FbxMeshBuilder(const FbxSkinBinding& oSkinBinding, std::vector<std::string>& vWarnings)
        : m_oSkinBinding(oSkinBinding)
        , m_vWarnings(vWarnings)
    {
    }

    std::optional<Assets::MeshData> FbxMeshBuilder::Build(const ufbx_node& oNode)
    {
        const ufbx_mesh& oSource = *oNode.mesh;
        const std::string sNodeName = UfbxConversion::ToString(oNode.name);
        m_oMesh.sName = !sNodeName.empty() ? sNodeName : UfbxConversion::ToString(oSource.name);
        m_pSkin = m_oSkinBinding.FindSkin(oNode);
        m_bHasTangents = oSource.vertex_tangent.exists && oSource.vertex_bitangent.exists && oSource.vertex_normal.exists;

        std::vector<uint32_t> vTriangleCorners(oSource.max_face_triangles * 3);
        const size_t uPartCount = oSource.material_parts.count;
        for (size_t uPart = 0; uPart < std::max<size_t>(uPartCount, 1); ++uPart)
        {
            const size_t uFirstIndex = m_oMesh.vIndices.size();
            if (uPartCount == 0)
            {
                for (size_t uFace = 0; uFace < oSource.faces.count; ++uFace)
                {
                    AppendFace(oSource, oSource.faces.data[uFace], vTriangleCorners);
                }
            }
            else
            {
                const ufbx_mesh_part& oPart = oSource.material_parts.data[uPart];
                for (size_t uFace = 0; uFace < oPart.face_indices.count; ++uFace)
                {
                    AppendFace(oSource, oSource.faces.data[oPart.face_indices.data[uFace]], vTriangleCorners);
                }
            }
            if (m_oMesh.vIndices.size() > uFirstIndex)
            {
                Assets::MeshSection oSection;
                oSection.sMaterialName = GetMaterialName(oNode, oSource, uPartCount == 0 ? 0 : oSource.material_parts.data[uPart].index);
                oSection.uFirstIndex = static_cast<uint32_t>(uFirstIndex);
                oSection.uIndexCount = static_cast<uint32_t>(m_oMesh.vIndices.size() - uFirstIndex);
                m_oMesh.vSections.push_back(std::move(oSection));
            }
            if (m_oMesh.vIndices.size() > Assets::MeshData::kMaxIndexCount)
            {
                throw std::runtime_error(std::format("mesh '{}' exceeds the index limit", m_oMesh.sName));
            }
        }

        if (m_oMesh.vIndices.empty())
        {
            m_vWarnings.push_back(std::format("mesh '{}' has no triangles and was skipped", m_oMesh.sName));
            return std::nullopt;
        }
        if (!oSource.vertex_uv.exists)
        {
            m_oMesh.vTexCoords0.clear();
        }
        if (!oSource.vertex_color.exists)
        {
            m_oMesh.vColorsAbgr.clear();
        }
        if (m_pSkin != nullptr && !m_bSkinRepresentable)
        {
            m_vWarnings.push_back(std::format("mesh '{}' references joints beyond descriptor index {}; skinning data dropped", m_oMesh.sName, kMaxSkinningJoint));
            m_oMesh.vJointIndices.clear();
            m_oMesh.vJointWeights.clear();
        }

        m_uCornerVertexCount = m_oMesh.vPositions.size();
        MeshWelder::Weld(m_oMesh);
        if (m_oMesh.vPositions.size() > Assets::MeshData::kMaxVertexCount)
        {
            throw std::runtime_error(std::format("mesh '{}' exceeds the vertex limit", m_oMesh.sName));
        }
        MeshTransformBaker::Apply(m_oMesh, m_oSkinBinding.GetBakeTransform(oNode), 0, 0);
        return std::move(m_oMesh);
    }

    size_t FbxMeshBuilder::GetCornerVertexCount() const noexcept
    {
        return m_uCornerVertexCount;
    }

    void FbxMeshBuilder::AppendFace(const ufbx_mesh& oSource, const ufbx_face& oFace, std::vector<uint32_t>& vTriangleCorners)
    {
        const uint32_t uTriangles = ufbx_triangulate_face(vTriangleCorners.data(), vTriangleCorners.size(), &oSource, oFace);
        for (uint32_t uCorner = 0; uCorner < uTriangles * 3; ++uCorner)
        {
            const uint32_t uIndex = vTriangleCorners[uCorner];
            m_oMesh.vIndices.push_back(static_cast<uint32_t>(m_oMesh.vPositions.size()));
            m_oMesh.vPositions.push_back(UfbxConversion::ToVector(ufbx_get_vertex_vec3(&oSource.vertex_position, uIndex)));
            const glm::vec3 vecNormal = oSource.vertex_normal.exists ? UfbxConversion::ToVector(ufbx_get_vertex_vec3(&oSource.vertex_normal, uIndex)) : glm::vec3(0.0f, 1.0f, 0.0f);
            m_oMesh.vNormals.push_back(vecNormal);
            if (m_bHasTangents)
            {
                const glm::vec3 vecTangent = UfbxConversion::ToVector(ufbx_get_vertex_vec3(&oSource.vertex_tangent, uIndex));
                const glm::vec3 vecBitangent = UfbxConversion::ToVector(ufbx_get_vertex_vec3(&oSource.vertex_bitangent, uIndex));
                const float fHandedness = glm::dot(glm::cross(vecNormal, vecTangent), vecBitangent) < 0.0f ? -1.0f : 1.0f;
                m_oMesh.vTangents.emplace_back(vecTangent, fHandedness);
            }
            if (oSource.vertex_uv.exists)
            {
                const ufbx_vec2 vecUv = ufbx_get_vertex_vec2(&oSource.vertex_uv, uIndex);
                m_oMesh.vTexCoords0.emplace_back(static_cast<float>(vecUv.x), 1.0f - static_cast<float>(vecUv.y));
            }
            if (oSource.vertex_color.exists)
            {
                const ufbx_vec4 vecColor = ufbx_get_vertex_vec4(&oSource.vertex_color, uIndex);
                m_oMesh.vColorsAbgr.push_back(ImportMath::PackColorAbgr(
                    glm::vec4(static_cast<float>(vecColor.x), static_cast<float>(vecColor.y), static_cast<float>(vecColor.z), static_cast<float>(vecColor.w))));
            }
            else
            {
                m_oMesh.vColorsAbgr.push_back(kOpaqueWhiteAbgr);
            }
            if (m_pSkin != nullptr)
            {
                AppendInfluences(oSource.vertex_indices.data[uIndex]);
            }
        }
    }

    void FbxMeshBuilder::AppendInfluences(uint32_t uVertex)
    {
        std::array<std::pair<float, uint32_t>, kMaxInfluences> aInfluences{};
        size_t uInfluenceCount = 0;
        if (uVertex < m_pSkin->vertices.count)
        {
            const ufbx_skin_vertex& oSkinVertex = m_pSkin->vertices.data[uVertex];
            std::vector<std::pair<float, uint32_t>> vCandidates;
            for (uint32_t uWeight = 0; uWeight < oSkinVertex.num_weights; ++uWeight)
            {
                const ufbx_skin_weight& oWeight = m_pSkin->weights.data[oSkinVertex.weight_begin + uWeight];
                const std::optional<uint32_t> optJoint = m_oSkinBinding.FindJoint(m_pSkin->clusters.data[oWeight.cluster_index]->bone_node);
                if (optJoint && oWeight.weight > 0.0)
                {
                    vCandidates.emplace_back(static_cast<float>(oWeight.weight), *optJoint);
                }
            }
            std::ranges::sort(vCandidates, [](const auto& oLeft, const auto& oRight) { return oLeft.first > oRight.first; });
            uInfluenceCount = std::min(vCandidates.size(), kMaxInfluences);
            std::copy_n(vCandidates.begin(), uInfluenceCount, aInfluences.begin());
        }

        glm::u8vec4 vecIndices(0);
        glm::vec4 vecWeights(0.0f);
        float fSum = 0.0f;
        for (size_t uInfluence = 0; uInfluence < uInfluenceCount; ++uInfluence)
        {
            const uint32_t uDescriptor = m_oSkinBinding.GetSkeleton().GetDescriptorIndex(aInfluences[uInfluence].second);
            m_bSkinRepresentable = m_bSkinRepresentable && uDescriptor <= kMaxSkinningJoint;
            vecIndices[static_cast<glm::length_t>(uInfluence)] = static_cast<uint8_t>(std::min(uDescriptor, kMaxSkinningJoint));
            vecWeights[static_cast<glm::length_t>(uInfluence)] = aInfluences[uInfluence].first;
            fSum += aInfluences[uInfluence].first;
        }
        m_oMesh.vJointIndices.push_back(vecIndices);
        m_oMesh.vJointWeights.push_back(fSum > kMinimumWeightSum ? vecWeights / fSum : glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
    }

    std::string FbxMeshBuilder::GetMaterialName(const ufbx_node& oNode, const ufbx_mesh& oSource, uint32_t uMaterial)
    {
        const ufbx_material* pMaterial = nullptr;
        if (uMaterial < oNode.materials.count)
        {
            pMaterial = oNode.materials.data[uMaterial];
        }
        else if (uMaterial < oSource.materials.count)
        {
            pMaterial = oSource.materials.data[uMaterial];
        }
        if (pMaterial == nullptr)
        {
            return std::string(Assets::MeshData::kDefaultSectionName);
        }
        const std::string sName = UfbxConversion::ToString(pMaterial->name);
        return sName.empty() ? std::format("Material{}", uMaterial) : sName;
    }
}
