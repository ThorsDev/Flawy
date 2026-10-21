#pragma once

#include <cstdint>

namespace Flawy::Renderer
{
    enum class MaterialPass : uint8_t
    {
        Forward,
        DepthOnly
    };

    enum class VertexFactoryKind : uint8_t
    {
        StaticMesh,
        StaticMeshTangent
    };

    struct MaterialPermutationKey
    {
        MaterialPass ePass = MaterialPass::Forward;
        VertexFactoryKind eVertexFactory = VertexFactoryKind::StaticMesh;
        uint32_t uStaticSwitchSet = 0;

        [[nodiscard]] bool operator==(const MaterialPermutationKey&) const noexcept = default;
    };
}
