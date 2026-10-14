#include "Flawy/Renderer/IndexBuffer.hpp"

#include "Renderer/Bgfx/BgfxResourceImpl.hpp"

#include <limits>
#include <stdexcept>

namespace Flawy::Renderer
{
    namespace
    {
        template <typename TIndex>
        constexpr IndexFormat kIndexFormatOf = sizeof(TIndex) == sizeof(uint32_t) ? IndexFormat::Uint32 : IndexFormat::Uint16;

        template <typename TIndex> uint32_t RequireIndexCount(std::span<const TIndex> spanIndices)
        {
            if (spanIndices.empty())
            {
                throw std::invalid_argument("Index data must not be empty");
            }
            if (spanIndices.size_bytes() > std::numeric_limits<uint32_t>::max())
            {
                throw std::invalid_argument("Index data is too large");
            }
            return static_cast<uint32_t>(spanIndices.size());
        }

        template <typename TIndex> const bgfx::Memory* CopyIndices(std::span<const TIndex> spanIndices)
        {
            return bgfx::copy(spanIndices.data(), static_cast<uint32_t>(spanIndices.size_bytes()));
        }

        template <typename TIndex> void CreateIndexBuffer(IndexBuffer::Impl& oImpl, std::span<const TIndex> spanIndices, BufferUsage eUsage)
        {
            const uint32_t uIndexCount = RequireIndexCount(spanIndices);
            const uint16_t uFlags = kIndexFormatOf<TIndex> == IndexFormat::Uint32 ? BGFX_BUFFER_INDEX32 : BGFX_BUFFER_NONE;
            const bgfx::Memory* pMemory = CopyIndices(spanIndices);

            bool bValid = false;
            if (eUsage == BufferUsage::Dynamic)
            {
                oImpl.hDynamic = bgfx::createDynamicIndexBuffer(pMemory, uFlags);
                bValid = bgfx::isValid(oImpl.hDynamic);
            }
            else
            {
                oImpl.hStatic = bgfx::createIndexBuffer(pMemory, uFlags);
                bValid = bgfx::isValid(oImpl.hStatic);
            }
            if (!bValid)
            {
                throw std::runtime_error("Failed to create index buffer");
            }

            oImpl.uIndexCount = uIndexCount;
            oImpl.eFormat = kIndexFormatOf<TIndex>;
            oImpl.eUsage = eUsage;
        }

        template <typename TIndex>
        void UpdateIndexBuffer(const IndexBuffer::Impl& oImpl, uint32_t uFirstIndex, std::span<const TIndex> spanIndices)
        {
            if (oImpl.eUsage != BufferUsage::Dynamic)
            {
                throw std::logic_error("Only dynamic index buffers can be updated");
            }
            if (oImpl.eFormat != kIndexFormatOf<TIndex>)
            {
                throw std::invalid_argument("Index buffer update uses a different index width than the buffer");
            }

            const uint32_t uIndexCount = RequireIndexCount(spanIndices);
            if (uFirstIndex > oImpl.uIndexCount || uIndexCount > oImpl.uIndexCount - uFirstIndex)
            {
                throw std::invalid_argument("Index buffer update exceeds the buffer size");
            }

            bgfx::update(oImpl.hDynamic, uFirstIndex, CopyIndices(spanIndices));
        }
    }

    IndexBuffer::IndexBuffer(std::span<const uint16_t> spanIndices, BufferUsage eUsage) : m_upImpl(std::make_unique<Impl>())
    {
        CreateIndexBuffer(*m_upImpl, spanIndices, eUsage);
    }

    IndexBuffer::IndexBuffer(std::span<const uint32_t> spanIndices, BufferUsage eUsage) : m_upImpl(std::make_unique<Impl>())
    {
        CreateIndexBuffer(*m_upImpl, spanIndices, eUsage);
    }

    IndexBuffer::~IndexBuffer() = default;

    void IndexBuffer::Update(uint32_t uFirstIndex, std::span<const uint16_t> spanIndices)
    {
        UpdateIndexBuffer(*m_upImpl, uFirstIndex, spanIndices);
    }

    void IndexBuffer::Update(uint32_t uFirstIndex, std::span<const uint32_t> spanIndices)
    {
        UpdateIndexBuffer(*m_upImpl, uFirstIndex, spanIndices);
    }

    uint32_t IndexBuffer::GetIndexCount() const noexcept
    {
        return m_upImpl->uIndexCount;
    }

    IndexFormat IndexBuffer::GetFormat() const noexcept
    {
        return m_upImpl->eFormat;
    }

    BufferUsage IndexBuffer::GetUsage() const noexcept
    {
        return m_upImpl->eUsage;
    }

    const IndexBuffer::Impl& IndexBuffer::GetImpl() const noexcept
    {
        return *m_upImpl;
    }
}
