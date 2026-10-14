#include "Flawy/Renderer/Texture.hpp"

#include "Renderer/Bgfx/BgfxRenderStateMapping.hpp"
#include "Renderer/Bgfx/BgfxResourceImpl.hpp"

#include <format>
#include <limits>
#include <stdexcept>
#include <string_view>

namespace Flawy::Renderer
{
    namespace
    {
        void ValidateDescriptor(const TextureDescriptor& oDescriptor, bool bHasPixels)
        {
            const uint32_t uMaxExtent = std::numeric_limits<uint16_t>::max();
            if (oDescriptor.uWidth == 0 || oDescriptor.uHeight == 0 || oDescriptor.uWidth > uMaxExtent || oDescriptor.uHeight > uMaxExtent)
            {
                throw std::invalid_argument(std::format("Texture size {}x{} must be between 1 and {} in each dimension", oDescriptor.uWidth,
                                                        oDescriptor.uHeight, uMaxExtent));
            }
            if (TextureFormatTraits::IsDepth(oDescriptor.eFormat) && (!oDescriptor.bRenderTarget || bHasPixels))
            {
                throw std::invalid_argument(std::format("Depth texture format {} requires a render target without initial pixels",
                                                        TextureFormatTraits::GetName(oDescriptor.eFormat)));
            }
        }

        void ValidatePixels(const TextureDescriptor& oDescriptor, std::span<const std::byte> spanPixels)
        {
            const uint64_t uExpectedSize =
                uint64_t{oDescriptor.uWidth} * oDescriptor.uHeight * TextureFormatTraits::GetBytesPerPixel(oDescriptor.eFormat);
            if (spanPixels.size() != uExpectedSize)
            {
                throw std::invalid_argument(std::format("Texture pixel data is {} bytes but a {}x{} {} texture needs {} bytes",
                                                        spanPixels.size(), oDescriptor.uWidth, oDescriptor.uHeight,
                                                        TextureFormatTraits::GetName(oDescriptor.eFormat), uExpectedSize));
            }
            if (uExpectedSize > std::numeric_limits<uint32_t>::max())
            {
                throw std::invalid_argument("Texture pixel data is too large");
            }
        }

        uint64_t BuildCreationFlags(const TextureDescriptor& oDescriptor) noexcept
        {
            uint64_t uFlags = BgfxRenderStateMapping::ToBgfxSamplerFlags(oDescriptor.oSampler);
            if (oDescriptor.bRenderTarget)
            {
                uFlags |= TextureFormatTraits::IsDepth(oDescriptor.eFormat) ? BGFX_TEXTURE_RT_WRITE_ONLY : BGFX_TEXTURE_RT;
            }
            return uFlags;
        }

        void CreateTexture(Texture::Impl& oImpl, const TextureDescriptor& oDescriptor, const bgfx::Memory* pMemory)
        {
            const bgfx::TextureFormat::Enum eBgfxFormat = BgfxRenderStateMapping::ToBgfxTextureFormat(oDescriptor.eFormat);
            const uint64_t uFlags = BuildCreationFlags(oDescriptor);
            if (!bgfx::isTextureValid(1, false, 1, eBgfxFormat, uFlags))
            {
                throw std::runtime_error(std::format("Texture format {} is not supported by the active renderer with the requested usage",
                                                     TextureFormatTraits::GetName(oDescriptor.eFormat)));
            }

            oImpl.hTexture = bgfx::createTexture2D(static_cast<uint16_t>(oDescriptor.uWidth), static_cast<uint16_t>(oDescriptor.uHeight),
                                                   false, 1, eBgfxFormat, uFlags, pMemory);
            if (!bgfx::isValid(oImpl.hTexture))
            {
                throw std::runtime_error(std::format("Failed to create {}x{} {} texture", oDescriptor.uWidth, oDescriptor.uHeight,
                                                     TextureFormatTraits::GetName(oDescriptor.eFormat)));
            }
            if (!oDescriptor.sDebugName.empty())
            {
                bgfx::setName(oImpl.hTexture, oDescriptor.sDebugName.c_str(), static_cast<int32_t>(oDescriptor.sDebugName.size()));
            }

            oImpl.uWidth = oDescriptor.uWidth;
            oImpl.uHeight = oDescriptor.uHeight;
            oImpl.eFormat = oDescriptor.eFormat;
            oImpl.oSampler = oDescriptor.oSampler;
            oImpl.bRenderTarget = oDescriptor.bRenderTarget;
        }
    }

    Texture::Texture(const TextureDescriptor& oDescriptor, std::span<const std::byte> spanPixels) : m_upImpl(std::make_unique<Impl>())
    {
        ValidateDescriptor(oDescriptor, true);
        ValidatePixels(oDescriptor, spanPixels);
        CreateTexture(*m_upImpl, oDescriptor, bgfx::copy(spanPixels.data(), static_cast<uint32_t>(spanPixels.size())));
    }

    Texture::Texture(const TextureDescriptor& oDescriptor) : m_upImpl(std::make_unique<Impl>())
    {
        ValidateDescriptor(oDescriptor, false);
        CreateTexture(*m_upImpl, oDescriptor, nullptr);
    }

    Texture::~Texture() = default;

    uint32_t Texture::GetWidth() const noexcept
    {
        return m_upImpl->uWidth;
    }

    uint32_t Texture::GetHeight() const noexcept
    {
        return m_upImpl->uHeight;
    }

    TextureFormat Texture::GetFormat() const noexcept
    {
        return m_upImpl->eFormat;
    }

    const SamplerState& Texture::GetSampler() const noexcept
    {
        return m_upImpl->oSampler;
    }

    bool Texture::IsRenderTarget() const noexcept
    {
        return m_upImpl->bRenderTarget;
    }

    const Texture::Impl& Texture::GetImpl() const noexcept
    {
        return *m_upImpl;
    }
}
