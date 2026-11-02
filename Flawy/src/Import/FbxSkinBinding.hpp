#pragma once

#include "Import/SkeletonAssembler.hpp"

#include <ufbx.h>

#include <glm/mat4x4.hpp>

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace Flawy::Import
{
    class FbxSkinBinding final
    {
    public:
        FbxSkinBinding(const ufbx_scene& oScene, std::vector<std::string>& vWarnings);

        void Build(const std::string& sDefaultName);

        [[nodiscard]] bool HasSkeleton() const noexcept;
        [[nodiscard]] const SkeletonAssembler& GetSkeleton() const;
        [[nodiscard]] const ufbx_skin_deformer* FindSkin(const ufbx_node& oMeshNode) const noexcept;
        [[nodiscard]] glm::mat4 GetBakeTransform(const ufbx_node& oMeshNode) const;
        [[nodiscard]] std::optional<uint32_t> FindJoint(const ufbx_node* pNode) const noexcept;

    private:
        void CollectSkinnedMeshes();
        void MergeDeformers();
        void BindMeshes(std::unordered_map<const ufbx_node*, ufbx_matrix>& mapInverseBinds);
        void BuildSkeleton(const std::string& sDefaultName, const std::unordered_map<const ufbx_node*, ufbx_matrix>& mapInverseBinds);
        [[nodiscard]] std::optional<uint32_t> FindParentJoint(const ufbx_node& oNode) const;
        [[nodiscard]] static const ufbx_skin_deformer* GetPrimaryDeformer(const ufbx_node& oMeshNode) noexcept;
        [[nodiscard]] static std::unordered_set<const ufbx_node*> CollectBones(const ufbx_skin_deformer& oDeformer);
        [[nodiscard]] static std::vector<const ufbx_node*> GetAncestry(const ufbx_node* pNode);
        [[nodiscard]] static const ufbx_node* FindCommonAncestor(const std::unordered_set<const ufbx_node*>& setBones);
        [[nodiscard]] static const ufbx_node* FindSkeletonRoot(const std::unordered_set<const ufbx_node*>& setBones);
        [[nodiscard]] static std::unordered_set<const ufbx_node*> CollectJointNodes(const std::unordered_set<const ufbx_node*>& setBones);

        const ufbx_scene& m_oScene;
        std::vector<std::string>& m_vWarnings;
        std::vector<const ufbx_node*> m_vSkinnedMeshNodes;
        std::unordered_set<const ufbx_skin_deformer*> m_setMergedDeformers;
        std::unordered_set<const ufbx_node*> m_setBones;
        std::unordered_map<const ufbx_node*, glm::mat4> m_mapBakeTransforms;
        std::vector<const ufbx_node*> m_vJointNodes;
        std::unordered_map<const ufbx_node*, uint32_t> m_mapNodeToJoint;
        std::optional<SkeletonAssembler> m_optSkeleton;
    };
}
