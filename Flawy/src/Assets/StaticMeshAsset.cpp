#include "Flawy/Assets/StaticMeshAsset.hpp"

#include "Assets/MeshVertexPacker.hpp"
#include "Flawy/Renderer/IndexBuffer.hpp"
#include "Flawy/Renderer/VertexBuffer.hpp"

#include <format>
#include <limits>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>

namespace Flawy::Assets
{
    namespace
    {
        std::string ChooseName(const AssetId& oId, const MeshData& oMeshData)
        {
            if (!oMeshData.sName.empty())
            {
                return oMeshData.sName;
            }
            return oId.GetPath();
        }

        MeshData PrepareMeshData(MeshData oMeshData)
        {
            oMeshData.Validate();
            if (oMeshData.vSections.empty())
            {
                oMeshData.vSections.push_back({std::string(MeshData::kDefaultSectionName), 0, static_cast<uint32_t>(oMeshData.vIndices.size())});
            }
            return oMeshData;
        }

        template <typename TIndex>
        std::unique_ptr<Renderer::IndexBuffer> CreateSectionIndexBuffer(const MeshData& oMeshData, const MeshSection& oSection)
        {
            const std::span<const uint32_t> spanSource = std::span<const uint32_t>(oMeshData.vIndices).subspan(oSection.uFirstIndex, oSection.uIndexCount);
            std::vector<TIndex> vIndices;
            vIndices.reserve(spanSource.size());
            for (const uint32_t uIndex : spanSource)
            {
                vIndices.push_back(static_cast<TIndex>(uIndex));
            }
            return std::make_unique<Renderer::IndexBuffer>(std::span<const TIndex>(vIndices));
        }
    }

    StaticMeshAsset::StaticMeshAsset(AssetId oId, MeshData oMeshData)
        : Asset(oId, ChooseName(oId, oMeshData))
        , m_oMeshData(PrepareMeshData(std::move(oMeshData)))
    {
        const PackedVertices oPacked = MeshVertexPacker::Pack(m_oMeshData);
        m_upVertexBuffer = std::make_unique<Renderer::VertexBuffer>(oPacked.vBytes, oPacked.oLayout);

        const bool bUse16BitIndices = m_oMeshData.GetVertexCount() <= std::numeric_limits<uint16_t>::max();
        m_vSectionIndexBuffers.reserve(m_oMeshData.vSections.size());
        for (const MeshSection& oSection : m_oMeshData.vSections)
        {
            m_vSectionIndexBuffers.push_back(bUse16BitIndices ? CreateSectionIndexBuffer<uint16_t>(m_oMeshData, oSection)
                                                              : CreateSectionIndexBuffer<uint32_t>(m_oMeshData, oSection));
        }
    }

    StaticMeshAsset::~StaticMeshAsset() = default;

    AssetTypeId StaticMeshAsset::GetTypeId() const noexcept
    {
        return kTypeId;
    }

    const MeshData& StaticMeshAsset::GetMeshData() const noexcept
    {
        return m_oMeshData;
    }

    const Renderer::VertexBuffer& StaticMeshAsset::GetVertexBuffer() const noexcept
    {
        return *m_upVertexBuffer;
    }

    uint32_t StaticMeshAsset::GetSectionCount() const noexcept
    {
        return static_cast<uint32_t>(m_vSectionIndexBuffers.size());
    }

    const Renderer::IndexBuffer& StaticMeshAsset::GetSectionIndexBuffer(uint32_t uSection) const
    {
        RequireSection(uSection);
        return *m_vSectionIndexBuffers[uSection];
    }

    const MeshSection& StaticMeshAsset::GetSection(uint32_t uSection) const
    {
        RequireSection(uSection);
        return m_oMeshData.vSections[uSection];
    }

    void StaticMeshAsset::RequireSection(uint32_t uSection) const
    {
        if (uSection >= m_vSectionIndexBuffers.size())
        {
            throw std::out_of_range(std::format("Static mesh '{}' has {} section(s); section {} does not exist", GetName(), m_vSectionIndexBuffers.size(), uSection));
        }
    }
}
