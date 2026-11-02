#pragma once

#include "Flawy/Assets/Asset.hpp"
#include "Flawy/Assets/MeshData.hpp"

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace Flawy::Renderer
{
    class IndexBuffer;
    class VertexBuffer;
}

namespace Flawy::Assets
{
    class StaticMeshAsset final : public Asset
    {
    public:
        static constexpr std::string_view kTypeName = "StaticMesh";
        static constexpr AssetTypeId kTypeId = AssetTypeId::FromName(kTypeName);
        static constexpr std::string_view kExtension = ".flmesh";

        StaticMeshAsset(AssetId oId, MeshData oMeshData);
        ~StaticMeshAsset() override;

        [[nodiscard]] AssetTypeId GetTypeId() const noexcept override;
        [[nodiscard]] const MeshData& GetMeshData() const noexcept;
        [[nodiscard]] const Renderer::VertexBuffer& GetVertexBuffer() const noexcept;
        [[nodiscard]] uint32_t GetSectionCount() const noexcept;
        [[nodiscard]] const Renderer::IndexBuffer& GetSectionIndexBuffer(uint32_t uSection) const;
        [[nodiscard]] const MeshSection& GetSection(uint32_t uSection) const;

    private:
        void RequireSection(uint32_t uSection) const;

        MeshData m_oMeshData;
        std::unique_ptr<Renderer::VertexBuffer> m_upVertexBuffer;
        std::vector<std::unique_ptr<Renderer::IndexBuffer>> m_vSectionIndexBuffers;
    };
}
