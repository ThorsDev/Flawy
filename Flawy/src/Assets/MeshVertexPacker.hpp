#pragma once

#include "Flawy/Assets/MeshData.hpp"
#include "Flawy/Renderer/VertexLayout.hpp"

#include <cstddef>
#include <vector>

namespace Flawy::Assets
{
    struct PackedVertices
    {
        std::vector<std::byte> vBytes;
        Renderer::VertexLayout oLayout;
    };

    class MeshVertexPacker final
    {
    public:
        MeshVertexPacker() = delete;

        [[nodiscard]] static PackedVertices Pack(const MeshData& oMesh);
        [[nodiscard]] static Renderer::VertexLayout BuildLayout(bool bWithTangent);
    };
}
