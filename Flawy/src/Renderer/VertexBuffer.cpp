#include "Flawy/Renderer/VertexBuffer.hpp"

#include "Renderer/Bgfx/BgfxResourceImpl.hpp"
#include "Renderer/Bgfx/BgfxVertexLayoutBuilder.hpp"

#include <limits>
#include <stdexcept>

namespace Flawy::Renderer
{
    VertexBuffer::VertexBuffer(std::span<const std::byte> spanData, const VertexLayout& oLayout)
        : m_upImpl(std::make_unique<Impl>())
    {
        const bgfx::VertexLayout oBgfxLayout = BgfxVertexLayoutBuilder().Build(oLayout);
        const uint32_t uStride = oBgfxLayout.getStride();

        if (spanData.empty() || spanData.size() % uStride != 0)
        {
            throw std::invalid_argument("Vertex data size must be a non-zero multiple of the vertex stride");
        }
        if (spanData.size() > std::numeric_limits<uint32_t>::max())
        {
            throw std::invalid_argument("Vertex data is too large");
        }

        const auto uSize = static_cast<uint32_t>(spanData.size());
        m_upImpl->hBuffer = bgfx::createVertexBuffer(bgfx::copy(spanData.data(), uSize), oBgfxLayout);
        if (!bgfx::isValid(m_upImpl->hBuffer))
        {
            throw std::runtime_error("Failed to create vertex buffer");
        }
        m_upImpl->uVertexCount = uSize / uStride;
    }

    VertexBuffer::~VertexBuffer() = default;

    uint32_t VertexBuffer::GetVertexCount() const noexcept
    {
        return m_upImpl->uVertexCount;
    }

    const VertexBuffer::Impl& VertexBuffer::GetImpl() const noexcept
    {
        return *m_upImpl;
    }
}
