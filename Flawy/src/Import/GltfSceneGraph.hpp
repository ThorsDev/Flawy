#pragma once

#include "Flawy/Animation/JointTransform.hpp"

#include <fastgltf/types.hpp>

#include <glm/mat4x4.hpp>

#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace Flawy::Import
{
    class GltfSceneGraph final
    {
    public:
        explicit GltfSceneGraph(const fastgltf::Asset& oAsset);

        [[nodiscard]] std::optional<size_t> GetParent(size_t uNode) const;
        [[nodiscard]] const glm::mat4& GetWorldMatrix(size_t uNode) const;
        [[nodiscard]] glm::mat4 GetParentWorldMatrix(size_t uNode) const;
        [[nodiscard]] std::span<const size_t> GetSceneNodes() const noexcept;

        [[nodiscard]] static Animation::JointTransform GetLocalTransform(const fastgltf::Node& oNode);

    private:
        void BuildHierarchy();
        void CollectSceneNodes();
        [[nodiscard]] static glm::mat4 GetLocalMatrix(const fastgltf::Node& oNode);

        const fastgltf::Asset& m_oAsset;
        std::vector<std::optional<size_t>> m_vParents;
        std::vector<glm::mat4> m_vWorld;
        std::vector<size_t> m_vSceneNodes;
    };
}
