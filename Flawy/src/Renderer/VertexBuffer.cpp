#include "Flawy/Renderer/VertexBuffer.hpp"

#include "Renderer/Bgfx/BgfxResourceImpl.hpp"
#include "Renderer/Bgfx/BgfxVertexLayoutBuilder.hpp"

#include <limits>
#include <stdexcept>

namespace Flawy::Renderer
{
    namespace
    {
        uint32_t RequireVertexDataSize(std::span<const std::byte> spanData, uint32_t uStride)
        {
            if (spanData.empty() || spanData.size() % uStride != 0)
            {
                throw std::invalid_argument("Vertex data size must be a non-zero multiple of the vertex stride");
            }
            if (spanData.size() > std::numeric_limits<uint32_t>::max())
            {
                throw std::invalid_argument("Vertex data is too large");
            }
            return static_cast<uint32_t>(spanData.size());
        }
    }

    VertexBuffer::VertexBuffer(std::span<const std::byte> spanData, const VertexLayout& oLayout, BufferUsage eUsage)
        : m_upImpl(std::make_unique<Impl>())
    {
        const bgfx::VertexLayout oBgfxLayout = BgfxVertexLayoutBuilder().Build(oLayout);
        const uint32_t uStride = oBgfxLayout.getStride();
        const uint32_t uSize = RequireVertexDataSize(spanData, uStride);
        const bgfx::Memory* pMemory = bgfx::copy(spanData.data(), uSize);

        bool bValid = false;
        if (eUsage == BufferUsage::Dynamic)
        {
            m_upImpl->hDynamic = bgfx::createDynamicVertexBuffer(pMemory, oBgfxLayout);
            bValid = bgfx::isValid(m_upImpl->hDynamic);
        }
        else
        {
            m_upImpl->hStatic = bgfx::createVertexBuffer(pMemory, oBgfxLayout);
            bValid = bgfx::isValid(m_upImpl->hStatic);
        }
        if (!bValid)
        {
            throw std::runtime_error("Failed to create vertex buffer");
        }

        m_upImpl->uVertexCount = uSize / uStride;
        m_upImpl->uStride = uStride;
        m_upImpl->eUsage = eUsage;
    }

    VertexBuffer::~VertexBuffer() = default;

    void VertexBuffer::Update(uint32_t uFirstVertex, std::span<const std::byte> spanData)
    {
        if (m_upImpl->eUsage != BufferUsage::Dynamic)
        {
            throw std::logic_error("Only dynamic vertex buffers can be updated");
        }

        const uint32_t uSize = RequireVertexDataSize(spanData, m_upImpl->uStride);
        const uint32_t uVertexCount = uSize / m_upImpl->uStride;
        if (uFirstVertex > m_upImpl->uVertexCount || uVertexCount > m_upImpl->uVertexCount - uFirstVertex)
        {
            throw std::invalid_argument("Vertex buffer update exceeds the buffer size");
        }

        bgfx::update(m_upImpl->hDynamic, uFirstVertex, bgfx::copy(spanData.data(), uSize));
    }

    uint32_t VertexBuffer::GetVertexCount() const noexcept
    {
        return m_upImpl->uVertexCount;
    }

    BufferUsage VertexBuffer::GetUsage() const noexcept
    {
        return m_upImpl->eUsage;
    }

    const VertexBuffer::Impl& VertexBuffer::GetImpl() const noexcept
    {
        return *m_upImpl;
    }
}
