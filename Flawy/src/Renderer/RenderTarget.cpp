#include "Flawy/Renderer/RenderTarget.hpp"

#include "Flawy/Renderer/Texture.hpp"
#include "Renderer/Bgfx/BgfxResourceImpl.hpp"

#include <array>
#include <format>
#include <stdexcept>

namespace Flawy::Renderer
{
    namespace
    {
        void ValidateDescriptor(const RenderTargetDescriptor& oDescriptor)
        {
            if (oDescriptor.uWidth == 0 || oDescriptor.uHeight == 0)
            {
                throw std::invalid_argument(std::format("Render target '{}' needs a non-zero size", oDescriptor.sDebugName));
            }
            if (TextureFormatTraits::IsDepth(oDescriptor.eColorFormat))
            {
                throw std::invalid_argument(std::format("Render target '{}' uses depth format {} as its color format",
                                                        oDescriptor.sDebugName,
                                                        TextureFormatTraits::GetName(oDescriptor.eColorFormat)));
            }
            if (oDescriptor.optDepthFormat && !TextureFormatTraits::IsDepth(*oDescriptor.optDepthFormat))
            {
                throw std::invalid_argument(std::format("Render target '{}' uses color format {} as its depth format",
                                                        oDescriptor.sDebugName,
                                                        TextureFormatTraits::GetName(*oDescriptor.optDepthFormat)));
            }
        }

        TextureDescriptor BuildAttachmentDescriptor(const RenderTargetDescriptor& oDescriptor, TextureFormat eFormat, std::string_view svSuffix)
        {
            TextureDescriptor oAttachment;
            oAttachment.uWidth = oDescriptor.uWidth;
            oAttachment.uHeight = oDescriptor.uHeight;
            oAttachment.eFormat = eFormat;
            oAttachment.oSampler = oDescriptor.oSampler;
            oAttachment.bRenderTarget = true;
            if (!oDescriptor.sDebugName.empty())
            {
                oAttachment.sDebugName = std::format("{}.{}", oDescriptor.sDebugName, svSuffix);
            }
            return oAttachment;
        }
    }

    RenderTarget::Impl::~Impl()
    {
        BgfxHandleRelease::Destroy(hFrameBuffer);
    }

    RenderTarget::RenderTarget(const RenderTargetDescriptor& oDescriptor)
        : m_upImpl(std::make_unique<Impl>())
    {
        ValidateDescriptor(oDescriptor);

        m_upImpl->oDescriptor = oDescriptor;
        m_upImpl->upColor = std::make_unique<Texture>(BuildAttachmentDescriptor(oDescriptor, oDescriptor.eColorFormat, "Color"));
        if (oDescriptor.optDepthFormat)
        {
            m_upImpl->upDepth = std::make_unique<Texture>(BuildAttachmentDescriptor(oDescriptor, *oDescriptor.optDepthFormat, "Depth"));
        }

        std::array<bgfx::TextureHandle, 2> aAttachments = {m_upImpl->upColor->GetImpl().hTexture, BGFX_INVALID_HANDLE};
        uint8_t uAttachmentCount = 1;
        if (m_upImpl->upDepth)
        {
            aAttachments[uAttachmentCount++] = m_upImpl->upDepth->GetImpl().hTexture;
        }

        m_upImpl->hFrameBuffer = bgfx::createFrameBuffer(uAttachmentCount, aAttachments.data(), false);
        if (!bgfx::isValid(m_upImpl->hFrameBuffer))
        {
            throw std::runtime_error(std::format("Failed to create frame buffer for render target '{}'", oDescriptor.sDebugName));
        }
        if (!oDescriptor.sDebugName.empty())
        {
            bgfx::setName(m_upImpl->hFrameBuffer, oDescriptor.sDebugName.c_str(), static_cast<int32_t>(oDescriptor.sDebugName.size()));
        }
    }

    RenderTarget::~RenderTarget() = default;

    const RenderTargetDescriptor& RenderTarget::GetDescriptor() const noexcept
    {
        return m_upImpl->oDescriptor;
    }

    uint32_t RenderTarget::GetWidth() const noexcept
    {
        return m_upImpl->oDescriptor.uWidth;
    }

    uint32_t RenderTarget::GetHeight() const noexcept
    {
        return m_upImpl->oDescriptor.uHeight;
    }

    const Texture& RenderTarget::GetColorTexture() const noexcept
    {
        return *m_upImpl->upColor;
    }

    bool RenderTarget::HasDepth() const noexcept
    {
        return m_upImpl->upDepth != nullptr;
    }

    const RenderTarget::Impl& RenderTarget::GetImpl() const noexcept
    {
        return *m_upImpl;
    }
}
