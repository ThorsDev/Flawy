#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace Flawy::Renderer
{
    enum class VertexAttribute : uint8_t
    {
        Position,
        Normal,
        Tangent,
        Bitangent,
        Color0,
        Color1,
        Color2,
        Color3,
        Indices,
        Weight,
        TexCoord0,
        TexCoord1,
        TexCoord2,
        TexCoord3,
        TexCoord4,
        TexCoord5,
        TexCoord6,
        TexCoord7
    };

    enum class VertexAttributeType : uint8_t
    {
        Uint8,
        Int16,
        Half,
        Float
    };

    struct VertexLayoutElement
    {
        VertexAttribute eAttribute = VertexAttribute::Position;
        uint8_t uCount = 0;
        VertexAttributeType eType = VertexAttributeType::Float;
        bool bNormalized = false;
    };

    class VertexLayout final
    {
    public:
        VertexLayout& Add(VertexAttribute eAttribute, uint8_t uCount, VertexAttributeType eType, bool bNormalized = false);

        [[nodiscard]] std::span<const VertexLayoutElement> GetElements() const noexcept;
        [[nodiscard]] uint32_t GetStride() const noexcept;

    private:
        std::vector<VertexLayoutElement> m_vElements;
    };
}
