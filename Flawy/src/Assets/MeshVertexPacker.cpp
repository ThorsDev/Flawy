#include "Assets/MeshVertexPacker.hpp"

#include <cstring>
#include <format>
#include <stdexcept>

namespace Flawy::Assets
{
    namespace
    {
        constexpr glm::vec3 kDefaultNormal = {0.0f, 1.0f, 0.0f};
        constexpr glm::vec2 kDefaultTexCoord = {0.0f, 0.0f};
        constexpr uint32_t kDefaultColorAbgr = 0xffffffffu;

        class VertexByteCursor final
        {
        public:
            explicit VertexByteCursor(std::vector<std::byte>& vBytes) noexcept
                : m_vBytes(vBytes)
            {
            }

            template <typename TValue>
            void Write(const TValue& oValue)
            {
                if (sizeof(TValue) > m_vBytes.size() - m_uOffset)
                {
                    throw std::logic_error(std::format("Vertex packing overflows the {} byte vertex buffer at offset {}",
                                                       m_vBytes.size(),
                                                       m_uOffset));
                }
                std::memcpy(m_vBytes.data() + m_uOffset, &oValue, sizeof(TValue));
                m_uOffset += sizeof(TValue);
            }

            template <typename TVector>
            void WriteVector(const TVector& vecValue)
            {
                for (typename TVector::length_type iComponent = 0; iComponent < TVector::length(); ++iComponent)
                {
                    Write(static_cast<float>(vecValue[iComponent]));
                }
            }

            [[nodiscard]] size_t GetOffset() const noexcept
            {
                return m_uOffset;
            }

        private:
            std::vector<std::byte>& m_vBytes;
            size_t m_uOffset = 0;
        };
    }

    PackedVertices MeshVertexPacker::Pack(const MeshData& oMesh)
    {
        const bool bWithTangent = oMesh.HasTangents();
        PackedVertices oPacked;
        oPacked.oLayout = BuildLayout(bWithTangent);

        const size_t uVertexCount = oMesh.vPositions.size();
        oPacked.vBytes.resize(uVertexCount * oPacked.oLayout.GetStride());

        VertexByteCursor oCursor(oPacked.vBytes);
        for (size_t uVertex = 0; uVertex < uVertexCount; ++uVertex)
        {
            oCursor.WriteVector(oMesh.vPositions[uVertex]);
            oCursor.WriteVector(oMesh.vNormals.empty() ? kDefaultNormal : oMesh.vNormals[uVertex]);
            if (bWithTangent)
            {
                oCursor.WriteVector(oMesh.vTangents[uVertex]);
            }
            oCursor.WriteVector(oMesh.vTexCoords0.empty() ? kDefaultTexCoord : oMesh.vTexCoords0[uVertex]);
            oCursor.Write(oMesh.vColorsAbgr.empty() ? kDefaultColorAbgr : oMesh.vColorsAbgr[uVertex]);
        }
        if (oCursor.GetOffset() != oPacked.vBytes.size())
        {
            throw std::logic_error(std::format("Vertex packing wrote {} of {} byte(s); the layout stride does not match the attributes",
                                               oCursor.GetOffset(),
                                               oPacked.vBytes.size()));
        }
        return oPacked;
    }

    Renderer::VertexLayout MeshVertexPacker::BuildLayout(bool bWithTangent)
    {
        Renderer::VertexLayout oLayout;
        oLayout.Add(Renderer::VertexAttribute::Position, 3, Renderer::VertexAttributeType::Float)
            .Add(Renderer::VertexAttribute::Normal, 3, Renderer::VertexAttributeType::Float);
        if (bWithTangent)
        {
            oLayout.Add(Renderer::VertexAttribute::Tangent, 4, Renderer::VertexAttributeType::Float);
        }
        oLayout.Add(Renderer::VertexAttribute::TexCoord0, 2, Renderer::VertexAttributeType::Float)
            .Add(Renderer::VertexAttribute::Color0, 4, Renderer::VertexAttributeType::Uint8, true);
        return oLayout;
    }
}
