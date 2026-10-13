#include "Flawy/Renderer/VertexLayout.hpp"

#include <array>
#include <cstddef>
#include <stdexcept>

namespace Flawy::Renderer
{
    namespace
    {
        constexpr uint8_t kMaxComponentCount = 4;

        constexpr std::array<std::array<uint8_t, kMaxComponentCount>, 4> kAttributeSizes = {{
            {1, 2, 4, 4},
            {2, 4, 8, 8},
            {2, 4, 8, 8},
            {4, 8, 12, 16},
        }};

        uint32_t GetAttributeSize(const VertexLayoutElement& oElement) noexcept
        {
            return kAttributeSizes[static_cast<size_t>(oElement.eType)][oElement.uCount - 1u];
        }
    }

    VertexLayout& VertexLayout::Add(VertexAttribute eAttribute, uint8_t uCount, VertexAttributeType eType, bool bNormalized)
    {
        if (uCount == 0 || uCount > kMaxComponentCount)
        {
            throw std::invalid_argument("Vertex attribute component count must be between 1 and 4");
        }

        m_vElements.push_back({eAttribute, uCount, eType, bNormalized});
        return *this;
    }

    std::span<const VertexLayoutElement> VertexLayout::GetElements() const noexcept
    {
        return m_vElements;
    }

    uint32_t VertexLayout::GetStride() const noexcept
    {
        uint32_t uStride = 0;
        for (const VertexLayoutElement& oElement : m_vElements)
        {
            uStride += GetAttributeSize(oElement);
        }
        return uStride;
    }
}
