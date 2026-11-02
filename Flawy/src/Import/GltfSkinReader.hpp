#pragma once

#include "Import/SkeletonAssembler.hpp"

#include <fastgltf/types.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace Flawy::Import
{
    class GltfAccessorReader;
    class GltfSceneGraph;

    class GltfSkinReader final
    {
    public:
        GltfSkinReader(const fastgltf::Asset& oAsset, const GltfSceneGraph& oGraph, const GltfAccessorReader& oAccessors, std::vector<std::string>& vWarnings);

        void Read(const std::string& sDefaultName);

        [[nodiscard]] bool HasSkeleton() const noexcept;
        [[nodiscard]] const SkeletonAssembler& GetSkeleton() const;
        [[nodiscard]] bool IsBoundToSkeleton(const fastgltf::Node& oNode) const noexcept;
        [[nodiscard]] std::optional<uint32_t> FindJoint(size_t uNode) const noexcept;

    private:
        void SelectSkin();
        void BuildSkeleton(const std::string& sDefaultName);
        [[nodiscard]] std::optional<uint32_t> FindParentJoint(size_t uNode) const;

        const fastgltf::Asset& m_oAsset;
        const GltfSceneGraph& m_oGraph;
        const GltfAccessorReader& m_oAccessors;
        std::vector<std::string>& m_vWarnings;
        std::optional<size_t> m_optSkin;
        std::optional<SkeletonAssembler> m_optSkeleton;
        std::unordered_map<size_t, uint32_t> m_mapNodeToJoint;
    };
}
