#include "Import/FbxSkinBinding.hpp"

#include "Import/UfbxConversion.hpp"

#include <glm/matrix.hpp>

#include <algorithm>
#include <cmath>
#include <format>
#include <stdexcept>
#include <utility>

namespace Flawy::Import
{
    namespace
    {
        constexpr double kBindTolerance = 1.0e-3;

        double GetMaximumDifference(const ufbx_matrix& matLeft, const ufbx_matrix& matRight) noexcept
        {
            double dDifference = 0.0;
            double dMagnitude = 1.0;
            for (size_t uElement = 0; uElement < 12; ++uElement)
            {
                dDifference = std::max(dDifference, std::abs(matLeft.v[uElement] - matRight.v[uElement]));
                dMagnitude = std::max(dMagnitude, std::abs(matRight.v[uElement]));
            }
            return dDifference / dMagnitude;
        }
    }

    FbxSkinBinding::FbxSkinBinding(const ufbx_scene& oScene, std::vector<std::string>& vWarnings)
        : m_oScene(oScene)
        , m_vWarnings(vWarnings)
    {
    }

    void FbxSkinBinding::Build(const std::string& sDefaultName)
    {
        CollectSkinnedMeshes();
        MergeDeformers();
        if (m_setBones.empty())
        {
            return;
        }

        std::unordered_map<const ufbx_node*, ufbx_matrix> mapInverseBinds;
        BindMeshes(mapInverseBinds);
        BuildSkeleton(sDefaultName, mapInverseBinds);
    }

    bool FbxSkinBinding::HasSkeleton() const noexcept
    {
        return m_optSkeleton.has_value();
    }

    const SkeletonAssembler& FbxSkinBinding::GetSkeleton() const
    {
        if (!m_optSkeleton)
        {
            throw std::logic_error("FBX skin binding has no skeleton");
        }
        return *m_optSkeleton;
    }

    const ufbx_skin_deformer* FbxSkinBinding::FindSkin(const ufbx_node& oMeshNode) const noexcept
    {
        const ufbx_skin_deformer* pDeformer = GetPrimaryDeformer(oMeshNode);
        if (!m_optSkeleton || pDeformer == nullptr || !m_setMergedDeformers.contains(pDeformer) || !m_mapBakeTransforms.contains(&oMeshNode))
        {
            return nullptr;
        }
        return pDeformer;
    }

    glm::mat4 FbxSkinBinding::GetBakeTransform(const ufbx_node& oMeshNode) const
    {
        const auto itTransform = m_mapBakeTransforms.find(&oMeshNode);
        if (FindSkin(oMeshNode) != nullptr && itTransform != m_mapBakeTransforms.end())
        {
            return itTransform->second;
        }
        return UfbxConversion::ToMatrix(oMeshNode.geometry_to_world);
    }

    std::optional<uint32_t> FbxSkinBinding::FindJoint(const ufbx_node* pNode) const noexcept
    {
        const auto itJoint = m_mapNodeToJoint.find(pNode);
        if (itJoint == m_mapNodeToJoint.end())
        {
            return std::nullopt;
        }
        return itJoint->second;
    }

    void FbxSkinBinding::CollectSkinnedMeshes()
    {
        for (size_t uNode = 0; uNode < m_oScene.nodes.count; ++uNode)
        {
            const ufbx_node* pNode = m_oScene.nodes.data[uNode];
            if (GetPrimaryDeformer(*pNode) == nullptr)
            {
                continue;
            }
            if (pNode->mesh->skin_deformers.count > 1)
            {
                m_vWarnings.push_back(std::format("mesh '{}' has {} skin deformers; only the first is used", UfbxConversion::ToString(pNode->name), pNode->mesh->skin_deformers.count));
            }
            m_vSkinnedMeshNodes.push_back(pNode);
        }
    }

    void FbxSkinBinding::MergeDeformers()
    {
        std::optional<const ufbx_node*> optSkeletonRoot;
        std::unordered_set<const ufbx_skin_deformer*> setVisited;
        for (const ufbx_node* pMeshNode : m_vSkinnedMeshNodes)
        {
            const ufbx_skin_deformer* pDeformer = GetPrimaryDeformer(*pMeshNode);
            if (!setVisited.insert(pDeformer).second)
            {
                continue;
            }

            const std::unordered_set<const ufbx_node*> setDeformerBones = CollectBones(*pDeformer);
            if (setDeformerBones.empty())
            {
                m_vWarnings.push_back(std::format("skin deformer '{}' has no bones and was ignored", UfbxConversion::ToString(pDeformer->name)));
                continue;
            }

            const ufbx_node* pRoot = FindSkeletonRoot(setDeformerBones);
            if (!optSkeletonRoot)
            {
                optSkeletonRoot = pRoot;
            }
            if (*optSkeletonRoot != pRoot)
            {
                m_vWarnings.push_back(std::format("skin deformer '{}' uses a different skeleton root and was not merged; its meshes are imported as static meshes",
                                                  UfbxConversion::ToString(pDeformer->name)));
                continue;
            }
            m_setMergedDeformers.insert(pDeformer);
            m_setBones.insert(setDeformerBones.begin(), setDeformerBones.end());
        }
    }

    void FbxSkinBinding::BindMeshes(std::unordered_map<const ufbx_node*, ufbx_matrix>& mapInverseBinds)
    {
        for (const ufbx_node* pMeshNode : m_vSkinnedMeshNodes)
        {
            const ufbx_skin_deformer* pDeformer = GetPrimaryDeformer(*pMeshNode);
            if (!m_setMergedDeformers.contains(pDeformer))
            {
                continue;
            }

            std::optional<ufbx_matrix> optBake;
            for (size_t uCluster = 0; uCluster < pDeformer->clusters.count && !optBake; ++uCluster)
            {
                const ufbx_skin_cluster* pCluster = pDeformer->clusters.data[uCluster];
                const auto itInverseBind = pCluster->bone_node != nullptr ? mapInverseBinds.find(pCluster->bone_node) : mapInverseBinds.end();
                if (itInverseBind != mapInverseBinds.end())
                {
                    const ufbx_matrix matBind = ufbx_matrix_invert(&itInverseBind->second);
                    optBake = ufbx_matrix_mul(&matBind, &pCluster->geometry_to_bone);
                }
            }
            for (size_t uCluster = 0; uCluster < pDeformer->clusters.count && !optBake; ++uCluster)
            {
                const ufbx_skin_cluster* pCluster = pDeformer->clusters.data[uCluster];
                if (pCluster->bone_node != nullptr)
                {
                    optBake = ufbx_matrix_mul(&pCluster->bind_to_world, &pCluster->geometry_to_bone);
                }
            }
            if (!optBake)
            {
                continue;
            }

            const ufbx_matrix matBakeInverse = ufbx_matrix_invert(&*optBake);
            bool bConflict = false;
            for (size_t uCluster = 0; uCluster < pDeformer->clusters.count; ++uCluster)
            {
                const ufbx_skin_cluster* pCluster = pDeformer->clusters.data[uCluster];
                if (pCluster->bone_node == nullptr)
                {
                    continue;
                }
                const ufbx_matrix matInverseBind = ufbx_matrix_mul(&pCluster->geometry_to_bone, &matBakeInverse);
                const auto [itInverseBind, bInserted] = mapInverseBinds.try_emplace(pCluster->bone_node, matInverseBind);
                if (!bInserted)
                {
                    const ufbx_matrix matExpected = ufbx_matrix_mul(&itInverseBind->second, &*optBake);
                    bConflict = bConflict || GetMaximumDifference(matExpected, pCluster->geometry_to_bone) > kBindTolerance;
                }
            }
            if (bConflict)
            {
                m_vWarnings.push_back(std::format("mesh '{}' was bound with a bind pose that conflicts with an earlier mesh; its skinning is approximated",
                                                  UfbxConversion::ToString(pMeshNode->name)));
            }
            m_mapBakeTransforms.emplace(pMeshNode, UfbxConversion::ToMatrix(*optBake));
        }
    }

    void FbxSkinBinding::BuildSkeleton(const std::string& sDefaultName, const std::unordered_map<const ufbx_node*, ufbx_matrix>& mapInverseBinds)
    {
        const std::unordered_set<const ufbx_node*> setJoints = CollectJointNodes(m_setBones);
        for (size_t uNode = 0; uNode < m_oScene.nodes.count; ++uNode)
        {
            const ufbx_node* pNode = m_oScene.nodes.data[uNode];
            if (setJoints.contains(pNode))
            {
                m_mapNodeToJoint.emplace(pNode, static_cast<uint32_t>(m_vJointNodes.size()));
                m_vJointNodes.push_back(pNode);
            }
        }

        std::vector<SkeletonSourceJoint> vJoints;
        vJoints.reserve(m_vJointNodes.size());
        for (const ufbx_node* pNode : m_vJointNodes)
        {
            SkeletonSourceJoint oJoint;
            oJoint.sName = UfbxConversion::ToString(pNode->name);
            oJoint.optParent = FindParentJoint(*pNode);
            const glm::mat4 matDirectParent = pNode->parent != nullptr ? UfbxConversion::ToMatrix(pNode->parent->node_to_world) : glm::mat4(1.0f);
            oJoint.matPrefix = oJoint.optParent ? glm::inverse(UfbxConversion::ToMatrix(m_vJointNodes[*oJoint.optParent]->node_to_world)) * matDirectParent : matDirectParent;
            oJoint.oLocalTransform = UfbxConversion::ToJointTransform(pNode->local_transform);
            const auto itInverseBind = mapInverseBinds.find(pNode);
            if (itInverseBind != mapInverseBinds.end())
            {
                oJoint.optInverseBindMatrix = UfbxConversion::ToMatrix(itInverseBind->second);
            }
            vJoints.push_back(std::move(oJoint));
        }

        const ufbx_skin_deformer* pFirstDeformer = GetPrimaryDeformer(*m_vSkinnedMeshNodes.front());
        const std::string sSkinName = pFirstDeformer != nullptr ? UfbxConversion::ToString(pFirstDeformer->name) : std::string();
        m_optSkeleton.emplace(sSkinName.empty() ? sDefaultName : sSkinName, std::move(vJoints), m_vWarnings);
    }

    std::optional<uint32_t> FbxSkinBinding::FindParentJoint(const ufbx_node& oNode) const
    {
        for (const ufbx_node* pAncestor = oNode.parent; pAncestor != nullptr; pAncestor = pAncestor->parent)
        {
            if (const std::optional<uint32_t> optJoint = FindJoint(pAncestor))
            {
                return optJoint;
            }
        }
        return std::nullopt;
    }

    const ufbx_skin_deformer* FbxSkinBinding::GetPrimaryDeformer(const ufbx_node& oMeshNode) noexcept
    {
        if (oMeshNode.mesh == nullptr || oMeshNode.mesh->skin_deformers.count == 0)
        {
            return nullptr;
        }
        return oMeshNode.mesh->skin_deformers.data[0];
    }

    std::unordered_set<const ufbx_node*> FbxSkinBinding::CollectBones(const ufbx_skin_deformer& oDeformer)
    {
        std::unordered_set<const ufbx_node*> setBones;
        for (size_t uCluster = 0; uCluster < oDeformer.clusters.count; ++uCluster)
        {
            const ufbx_node* pBone = oDeformer.clusters.data[uCluster]->bone_node;
            if (pBone != nullptr)
            {
                setBones.insert(pBone);
            }
        }
        return setBones;
    }

    std::vector<const ufbx_node*> FbxSkinBinding::GetAncestry(const ufbx_node* pNode)
    {
        std::vector<const ufbx_node*> vChain;
        for (const ufbx_node* pCurrent = pNode; pCurrent != nullptr; pCurrent = pCurrent->parent)
        {
            vChain.push_back(pCurrent);
        }
        std::ranges::reverse(vChain);
        return vChain;
    }

    const ufbx_node* FbxSkinBinding::FindCommonAncestor(const std::unordered_set<const ufbx_node*>& setBones)
    {
        std::vector<const ufbx_node*> vCommon;
        bool bFirst = true;
        for (const ufbx_node* pBone : setBones)
        {
            const std::vector<const ufbx_node*> vChain = GetAncestry(pBone);
            if (bFirst)
            {
                vCommon = vChain;
                bFirst = false;
                continue;
            }
            size_t uShared = 0;
            while (uShared < vCommon.size() && uShared < vChain.size() && vCommon[uShared] == vChain[uShared])
            {
                ++uShared;
            }
            vCommon.resize(uShared);
        }
        return vCommon.empty() ? nullptr : vCommon.back();
    }

    const ufbx_node* FbxSkinBinding::FindSkeletonRoot(const std::unordered_set<const ufbx_node*>& setBones)
    {
        const ufbx_node* pRoot = FindCommonAncestor(setBones);
        while (pRoot != nullptr && !pRoot->is_root && pRoot->parent != nullptr && !pRoot->parent->is_root)
        {
            pRoot = pRoot->parent;
        }
        return pRoot;
    }

    std::unordered_set<const ufbx_node*> FbxSkinBinding::CollectJointNodes(const std::unordered_set<const ufbx_node*>& setBones)
    {
        const ufbx_node* pCommonAncestor = FindCommonAncestor(setBones);
        std::unordered_set<const ufbx_node*> setJoints;
        for (const ufbx_node* pBone : setBones)
        {
            for (const ufbx_node* pCurrent = pBone; pCurrent != nullptr && !pCurrent->is_root; pCurrent = pCurrent->parent)
            {
                setJoints.insert(pCurrent);
                if (pCurrent == pCommonAncestor)
                {
                    break;
                }
            }
        }
        return setJoints;
    }
}
