#pragma once

#include "Flawy/Assets/MeshData.hpp"

#include <fastgltf/types.hpp>

#include <glm/mat4x4.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Flawy::Import
{
    class GltfAccessorReader;
    class SkeletonAssembler;

    class GltfMeshBuilder final
    {
    public:
        GltfMeshBuilder(const fastgltf::Asset& oAsset,
                        const GltfAccessorReader& oAccessors,
                        const SkeletonAssembler* pSkeleton,
                        std::vector<std::string>& vWarnings);

        [[nodiscard]] std::optional<Assets::MeshData> Build(size_t uNode, const std::optional<glm::mat4>& optBakeTransform);

    private:
        void AppendPrimitive(const fastgltf::Primitive& oPrimitive, size_t uPrimitive);
        template <typename TElement>
        bool AppendStream(const fastgltf::Primitive& oPrimitive, std::string_view svAttribute, size_t uVertexCount, std::vector<TElement>& vTarget, const TElement& oDefault);
        bool AppendColors(const fastgltf::Primitive& oPrimitive, size_t uVertexCount);
        bool AppendSkinning(const fastgltf::Primitive& oPrimitive, size_t uVertexCount);
        void AppendIndices(const fastgltf::Primitive& oPrimitive, size_t uFirstVertex, size_t uVertexCount);
        void FinishStreams();
        [[nodiscard]] std::string GetMaterialName(const fastgltf::Primitive& oPrimitive) const;

        const fastgltf::Asset& m_oAsset;
        const GltfAccessorReader& m_oAccessors;
        const SkeletonAssembler* m_pSkeleton;
        std::vector<std::string>& m_vWarnings;
        Assets::MeshData m_oMesh;
        bool m_bAnyTexCoords = false;
        bool m_bAnyColors = false;
        bool m_bAllTangents = true;
        bool m_bAllSkinning = true;
    };
}
