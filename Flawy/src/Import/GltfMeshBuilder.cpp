#include "Import/GltfMeshBuilder.hpp"

#include "Import/GltfAccessorReader.hpp"
#include "Import/ImportMath.hpp"
#include "Import/MeshTransformBaker.hpp"
#include "Import/NormalGenerator.hpp"
#include "Import/SkeletonAssembler.hpp"

#include <fastgltf/glm_element_traits.hpp>

#include <glm/common.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <format>
#include <limits>
#include <stdexcept>
#include <utility>

namespace Flawy::Import
{
    namespace
    {
        constexpr uint32_t kMaxSkinningJoint = std::numeric_limits<uint8_t>::max();
        constexpr uint32_t kOpaqueWhiteAbgr = 0xffffffffu;
        constexpr float kMinimumWeightSum = 1.0e-6f;
    }

    GltfMeshBuilder::GltfMeshBuilder(const fastgltf::Asset& oAsset,
                                     const GltfAccessorReader& oAccessors,
                                     const SkeletonAssembler* pSkeleton,
                                     std::vector<std::string>& vWarnings)
        : m_oAsset(oAsset)
        , m_oAccessors(oAccessors)
        , m_pSkeleton(pSkeleton)
        , m_vWarnings(vWarnings)
    {
    }

    std::optional<Assets::MeshData> GltfMeshBuilder::Build(size_t uNode, const std::optional<glm::mat4>& optBakeTransform)
    {
        const fastgltf::Node& oNode = m_oAsset.nodes[uNode];
        if (!oNode.meshIndex || *oNode.meshIndex >= m_oAsset.meshes.size())
        {
            throw std::runtime_error(std::format("node {} references a missing mesh", uNode));
        }
        const fastgltf::Mesh& oSourceMesh = m_oAsset.meshes[*oNode.meshIndex];

        const std::string sNodeName(std::string_view(oNode.name));
        const std::string sMeshName(std::string_view(oSourceMesh.name));
        m_oMesh.sName = !sNodeName.empty() ? sNodeName : !sMeshName.empty() ? sMeshName : std::format("Mesh{}", *oNode.meshIndex);
        for (size_t uPrimitive = 0; uPrimitive < oSourceMesh.primitives.size(); ++uPrimitive)
        {
            AppendPrimitive(oSourceMesh.primitives[uPrimitive], uPrimitive);
        }

        if (m_oMesh.vIndices.empty())
        {
            m_vWarnings.push_back(std::format("mesh '{}' has no triangles and was skipped", m_oMesh.sName));
            return std::nullopt;
        }
        FinishStreams();
        if (optBakeTransform)
        {
            MeshTransformBaker::Apply(m_oMesh, *optBakeTransform, 0, 0);
        }
        return std::move(m_oMesh);
    }

    void GltfMeshBuilder::FinishStreams()
    {
        if (!m_bAnyTexCoords)
        {
            m_oMesh.vTexCoords0.clear();
        }
        if (!m_bAnyColors)
        {
            m_oMesh.vColorsAbgr.clear();
        }
        if (!m_bAllTangents)
        {
            m_oMesh.vTangents.clear();
        }
        if (m_pSkeleton != nullptr && !m_bAllSkinning)
        {
            m_vWarnings.push_back(std::format("skinned mesh '{}' has primitives without usable JOINTS_0/WEIGHTS_0; skinning data dropped", m_oMesh.sName));
        }
        if (m_pSkeleton == nullptr || !m_bAllSkinning)
        {
            m_oMesh.vJointIndices.clear();
            m_oMesh.vJointWeights.clear();
        }
    }

    void GltfMeshBuilder::AppendPrimitive(const fastgltf::Primitive& oPrimitive, size_t uPrimitive)
    {
        if (oPrimitive.type != fastgltf::PrimitiveType::Triangles && oPrimitive.type != fastgltf::PrimitiveType::TriangleStrip &&
            oPrimitive.type != fastgltf::PrimitiveType::TriangleFan)
        {
            m_vWarnings.push_back(std::format("mesh '{}' primitive {} is not a triangle primitive and was skipped", m_oMesh.sName, uPrimitive));
            return;
        }
        const auto* itPosition = oPrimitive.findAttribute("POSITION");
        if (itPosition == oPrimitive.attributes.end())
        {
            m_vWarnings.push_back(std::format("mesh '{}' primitive {} has no POSITION attribute and was skipped", m_oMesh.sName, uPrimitive));
            return;
        }

        const size_t uFirstVertex = m_oMesh.vPositions.size();
        const size_t uFirstIndex = m_oMesh.vIndices.size();
        const std::vector<glm::vec3> vPositions = m_oAccessors.Read<glm::vec3>(itPosition->accessorIndex, "POSITION");
        if (uFirstVertex + vPositions.size() > Assets::MeshData::kMaxVertexCount)
        {
            throw std::runtime_error(std::format("mesh '{}' exceeds {} vertices", m_oMesh.sName, Assets::MeshData::kMaxVertexCount));
        }
        const size_t uVertexCount = vPositions.size();
        m_oMesh.vPositions.insert(m_oMesh.vPositions.end(), vPositions.begin(), vPositions.end());

        const bool bHasNormals = AppendStream<glm::vec3>(oPrimitive, "NORMAL", uVertexCount, m_oMesh.vNormals, glm::vec3(0.0f, 1.0f, 0.0f));
        m_bAllTangents = AppendStream<glm::vec4>(oPrimitive, "TANGENT", uVertexCount, m_oMesh.vTangents, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)) && m_bAllTangents;
        m_bAnyTexCoords = AppendStream<glm::vec2>(oPrimitive, "TEXCOORD_0", uVertexCount, m_oMesh.vTexCoords0, glm::vec2(0.0f)) || m_bAnyTexCoords;
        m_bAnyColors = AppendColors(oPrimitive, uVertexCount) || m_bAnyColors;
        if (m_pSkeleton != nullptr)
        {
            m_bAllSkinning = AppendSkinning(oPrimitive, uVertexCount) && m_bAllSkinning;
        }

        AppendIndices(oPrimitive, uFirstVertex, uVertexCount);
        if (!bHasNormals)
        {
            NormalGenerator::Generate(m_oMesh, uFirstVertex, uFirstIndex);
        }

        Assets::MeshSection oSection;
        oSection.sMaterialName = GetMaterialName(oPrimitive);
        oSection.uFirstIndex = static_cast<uint32_t>(uFirstIndex);
        oSection.uIndexCount = static_cast<uint32_t>(m_oMesh.vIndices.size() - uFirstIndex);
        if (oSection.uIndexCount != 0)
        {
            m_oMesh.vSections.push_back(std::move(oSection));
        }
    }

    template <typename TElement>
    bool GltfMeshBuilder::AppendStream(const fastgltf::Primitive& oPrimitive,
                                       std::string_view svAttribute,
                                       size_t uVertexCount,
                                       std::vector<TElement>& vTarget,
                                       const TElement& oDefault)
    {
        const auto* itAttribute = oPrimitive.findAttribute(svAttribute);
        if (itAttribute != oPrimitive.attributes.end())
        {
            std::vector<TElement> vValues = m_oAccessors.Read<TElement>(itAttribute->accessorIndex, svAttribute);
            if (vValues.size() == uVertexCount)
            {
                vTarget.insert(vTarget.end(), vValues.begin(), vValues.end());
                return true;
            }
            m_vWarnings.push_back(std::format("mesh '{}' attribute {} has {} elements for {} vertices and was ignored", m_oMesh.sName, svAttribute, vValues.size(), uVertexCount));
        }
        vTarget.insert(vTarget.end(), uVertexCount, oDefault);
        return false;
    }

    bool GltfMeshBuilder::AppendColors(const fastgltf::Primitive& oPrimitive, size_t uVertexCount)
    {
        const auto* itAttribute = oPrimitive.findAttribute("COLOR_0");
        if (itAttribute != oPrimitive.attributes.end())
        {
            std::vector<glm::vec4> vColors;
            if (m_oAccessors.GetType(itAttribute->accessorIndex, "COLOR_0") == fastgltf::AccessorType::Vec3)
            {
                for (const glm::vec3& vecColor : m_oAccessors.Read<glm::vec3>(itAttribute->accessorIndex, "COLOR_0"))
                {
                    vColors.emplace_back(vecColor, 1.0f);
                }
            }
            else
            {
                vColors = m_oAccessors.Read<glm::vec4>(itAttribute->accessorIndex, "COLOR_0");
            }
            if (vColors.size() == uVertexCount)
            {
                for (const glm::vec4& vecColor : vColors)
                {
                    m_oMesh.vColorsAbgr.push_back(ImportMath::PackColorAbgr(vecColor));
                }
                return true;
            }
        }
        m_oMesh.vColorsAbgr.insert(m_oMesh.vColorsAbgr.end(), uVertexCount, kOpaqueWhiteAbgr);
        return false;
    }

    bool GltfMeshBuilder::AppendSkinning(const fastgltf::Primitive& oPrimitive, size_t uVertexCount)
    {
        const auto* itJoints = oPrimitive.findAttribute("JOINTS_0");
        const auto* itWeights = oPrimitive.findAttribute("WEIGHTS_0");
        const bool bPresent = itJoints != oPrimitive.attributes.end() && itWeights != oPrimitive.attributes.end();
        std::vector<glm::u16vec4> vJoints;
        std::vector<glm::vec4> vWeights;
        if (bPresent)
        {
            vJoints = m_oAccessors.Read<glm::u16vec4>(itJoints->accessorIndex, "JOINTS_0");
            vWeights = m_oAccessors.Read<glm::vec4>(itWeights->accessorIndex, "WEIGHTS_0");
        }
        if (!bPresent || vJoints.size() != uVertexCount || vWeights.size() != uVertexCount)
        {
            m_oMesh.vJointIndices.insert(m_oMesh.vJointIndices.end(), uVertexCount, glm::u8vec4(0));
            m_oMesh.vJointWeights.insert(m_oMesh.vJointWeights.end(), uVertexCount, glm::vec4(1.0f, 0.0f, 0.0f, 0.0f));
            return false;
        }

        bool bRepresentable = true;
        for (size_t uVertex = 0; uVertex < uVertexCount; ++uVertex)
        {
            glm::u8vec4 vecIndices(0);
            glm::vec4 vecWeights = glm::max(vWeights[uVertex], glm::vec4(0.0f));
            for (glm::length_t iComponent = 0; iComponent < 4; ++iComponent)
            {
                const uint32_t uSourceJoint = vJoints[uVertex][iComponent];
                if (uSourceJoint >= m_pSkeleton->GetJointCount())
                {
                    throw std::runtime_error(std::format("JOINTS_0 references joint {} but the skin has {} joint(s)", uSourceJoint, m_pSkeleton->GetJointCount()));
                }
                if (vecWeights[iComponent] <= 0.0f)
                {
                    continue;
                }
                const uint32_t uDescriptor = m_pSkeleton->GetDescriptorIndex(uSourceJoint);
                bRepresentable = bRepresentable && uDescriptor <= kMaxSkinningJoint;
                vecIndices[iComponent] = static_cast<uint8_t>(std::min(uDescriptor, kMaxSkinningJoint));
            }
            const float fSum = vecWeights.x + vecWeights.y + vecWeights.z + vecWeights.w;
            vecWeights = fSum > kMinimumWeightSum ? vecWeights / fSum : glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
            m_oMesh.vJointIndices.push_back(vecIndices);
            m_oMesh.vJointWeights.push_back(vecWeights);
        }
        if (!bRepresentable)
        {
            m_vWarnings.push_back(std::format("mesh '{}' references joints beyond descriptor index {}", m_oMesh.sName, kMaxSkinningJoint));
        }
        return bRepresentable;
    }

    void GltfMeshBuilder::AppendIndices(const fastgltf::Primitive& oPrimitive, size_t uFirstVertex, size_t uVertexCount)
    {
        std::vector<uint32_t> vSource;
        if (oPrimitive.indicesAccessor)
        {
            vSource = m_oAccessors.Read<uint32_t>(*oPrimitive.indicesAccessor, "indices");
        }
        else
        {
            vSource.resize(uVertexCount);
            for (size_t uVertex = 0; uVertex < uVertexCount; ++uVertex)
            {
                vSource[uVertex] = static_cast<uint32_t>(uVertex);
            }
        }
        for (const uint32_t uIndex : vSource)
        {
            if (uIndex >= uVertexCount)
            {
                throw std::runtime_error(std::format("mesh '{}' index {} is out of range ({} vertices)", m_oMesh.sName, uIndex, uVertexCount));
            }
        }

        const auto fnPush = [this, uFirstVertex](uint32_t uA, uint32_t uB, uint32_t uC) {
            m_oMesh.vIndices.push_back(static_cast<uint32_t>(uFirstVertex) + uA);
            m_oMesh.vIndices.push_back(static_cast<uint32_t>(uFirstVertex) + uB);
            m_oMesh.vIndices.push_back(static_cast<uint32_t>(uFirstVertex) + uC);
        };
        if (oPrimitive.type == fastgltf::PrimitiveType::Triangles)
        {
            for (size_t uIndex = 0; uIndex + 2 < vSource.size(); uIndex += 3)
            {
                fnPush(vSource[uIndex], vSource[uIndex + 1], vSource[uIndex + 2]);
            }
        }
        else if (oPrimitive.type == fastgltf::PrimitiveType::TriangleStrip)
        {
            for (size_t uIndex = 0; uIndex + 2 < vSource.size(); ++uIndex)
            {
                const bool bEven = uIndex % 2 == 0;
                fnPush(vSource[bEven ? uIndex : uIndex + 1], vSource[bEven ? uIndex + 1 : uIndex], vSource[uIndex + 2]);
            }
        }
        else
        {
            for (size_t uIndex = 1; uIndex + 1 < vSource.size(); ++uIndex)
            {
                fnPush(vSource[0], vSource[uIndex], vSource[uIndex + 1]);
            }
        }
        if (m_oMesh.vIndices.size() > Assets::MeshData::kMaxIndexCount)
        {
            throw std::runtime_error(std::format("mesh '{}' exceeds {} indices", m_oMesh.sName, Assets::MeshData::kMaxIndexCount));
        }
    }

    std::string GltfMeshBuilder::GetMaterialName(const fastgltf::Primitive& oPrimitive) const
    {
        if (!oPrimitive.materialIndex || *oPrimitive.materialIndex >= m_oAsset.materials.size())
        {
            return std::string(Assets::MeshData::kDefaultSectionName);
        }
        const std::string sName(std::string_view(m_oAsset.materials[*oPrimitive.materialIndex].name));
        return sName.empty() ? std::format("Material{}", *oPrimitive.materialIndex) : sName;
    }
}
