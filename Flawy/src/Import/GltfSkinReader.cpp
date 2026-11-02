#include "Import/GltfSkinReader.hpp"

#include "Import/GltfAccessorReader.hpp"
#include "Import/GltfSceneGraph.hpp"

#include <fastgltf/glm_element_traits.hpp>

#include <glm/matrix.hpp>

#include <algorithm>
#include <format>
#include <stdexcept>
#include <string_view>
#include <utility>

namespace Flawy::Import
{
    GltfSkinReader::GltfSkinReader(const fastgltf::Asset& oAsset, const GltfSceneGraph& oGraph, const GltfAccessorReader& oAccessors, std::vector<std::string>& vWarnings)
        : m_oAsset(oAsset)
        , m_oGraph(oGraph)
        , m_oAccessors(oAccessors)
        , m_vWarnings(vWarnings)
    {
    }

    void GltfSkinReader::Read(const std::string& sDefaultName)
    {
        SelectSkin();
        if (m_optSkin)
        {
            BuildSkeleton(sDefaultName);
        }
    }

    bool GltfSkinReader::HasSkeleton() const noexcept
    {
        return m_optSkeleton.has_value();
    }

    const SkeletonAssembler& GltfSkinReader::GetSkeleton() const
    {
        if (!m_optSkeleton)
        {
            throw std::logic_error("glTF skin reader has no skeleton");
        }
        return *m_optSkeleton;
    }

    bool GltfSkinReader::IsBoundToSkeleton(const fastgltf::Node& oNode) const noexcept
    {
        return m_optSkeleton && m_optSkin && oNode.skinIndex && *oNode.skinIndex == *m_optSkin;
    }

    std::optional<uint32_t> GltfSkinReader::FindJoint(size_t uNode) const noexcept
    {
        const auto itJoint = m_mapNodeToJoint.find(uNode);
        if (itJoint == m_mapNodeToJoint.end())
        {
            return std::nullopt;
        }
        return itJoint->second;
    }

    void GltfSkinReader::SelectSkin()
    {
        if (m_oAsset.skins.empty())
        {
            return;
        }

        for (const size_t uNode : m_oGraph.GetSceneNodes())
        {
            const fastgltf::Node& oNode = m_oAsset.nodes[uNode];
            if (oNode.meshIndex && oNode.skinIndex)
            {
                m_optSkin = *oNode.skinIndex;
                break;
            }
        }
        if (!m_optSkin)
        {
            m_optSkin = 0;
        }
        if (*m_optSkin >= m_oAsset.skins.size())
        {
            throw std::runtime_error(std::format("node references missing skin {}", *m_optSkin));
        }
        if (m_oAsset.skins.size() > 1)
        {
            m_vWarnings.push_back(std::format("the file contains {} skins; only skin {} ('{}') is imported, meshes bound to other skins are imported in their bind pose without skinning data",
                                              m_oAsset.skins.size(),
                                              *m_optSkin,
                                              std::string_view(m_oAsset.skins[*m_optSkin].name)));
        }
    }

    void GltfSkinReader::BuildSkeleton(const std::string& sDefaultName)
    {
        const fastgltf::Skin& oSkin = m_oAsset.skins[*m_optSkin];
        if (oSkin.joints.empty())
        {
            m_vWarnings.push_back(std::format("skin {} has no joints; no skeleton is imported", *m_optSkin));
            m_optSkin.reset();
            return;
        }

        for (size_t uJoint = 0; uJoint < oSkin.joints.size(); ++uJoint)
        {
            const size_t uNode = oSkin.joints[uJoint];
            if (uNode >= m_oAsset.nodes.size() || !m_mapNodeToJoint.emplace(uNode, static_cast<uint32_t>(uJoint)).second)
            {
                throw std::runtime_error(std::format("skin {} joint {} references an invalid or duplicate node {}", *m_optSkin, uJoint, uNode));
            }
        }

        std::vector<glm::mat4> vInverseBinds(oSkin.joints.size(), glm::mat4(1.0f));
        if (oSkin.inverseBindMatrices)
        {
            const std::vector<glm::mat4> vSource = m_oAccessors.Read<glm::mat4>(*oSkin.inverseBindMatrices, "inverse bind matrices");
            if (vSource.size() < vInverseBinds.size())
            {
                throw std::runtime_error(std::format("skin {} has {} inverse bind matrices for {} joints", *m_optSkin, vSource.size(), vInverseBinds.size()));
            }
            std::copy_n(vSource.begin(), vInverseBinds.size(), vInverseBinds.begin());
        }

        std::vector<SkeletonSourceJoint> vJoints;
        vJoints.reserve(oSkin.joints.size());
        for (size_t uJoint = 0; uJoint < oSkin.joints.size(); ++uJoint)
        {
            const size_t uNode = oSkin.joints[uJoint];
            SkeletonSourceJoint oJoint;
            oJoint.sName = std::string(std::string_view(m_oAsset.nodes[uNode].name));
            oJoint.optParent = FindParentJoint(uNode);
            const glm::mat4 matDirectParent = m_oGraph.GetParentWorldMatrix(uNode);
            oJoint.matPrefix = oJoint.optParent ? glm::inverse(m_oGraph.GetWorldMatrix(oSkin.joints[*oJoint.optParent])) * matDirectParent : matDirectParent;
            oJoint.oLocalTransform = GltfSceneGraph::GetLocalTransform(m_oAsset.nodes[uNode]);
            oJoint.optInverseBindMatrix = vInverseBinds[uJoint];
            vJoints.push_back(std::move(oJoint));
        }

        const std::string sSkinName(std::string_view(oSkin.name));
        m_optSkeleton.emplace(sSkinName.empty() ? sDefaultName : sSkinName, std::move(vJoints), m_vWarnings);
    }

    std::optional<uint32_t> GltfSkinReader::FindParentJoint(size_t uNode) const
    {
        std::optional<size_t> optAncestor = m_oGraph.GetParent(uNode);
        while (optAncestor)
        {
            if (const std::optional<uint32_t> optJoint = FindJoint(*optAncestor))
            {
                return optJoint;
            }
            optAncestor = m_oGraph.GetParent(*optAncestor);
        }
        return std::nullopt;
    }
}
