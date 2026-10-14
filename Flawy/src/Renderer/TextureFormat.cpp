#include "Flawy/Renderer/TextureFormat.hpp"

#include <array>
#include <cstddef>

namespace Flawy::Renderer
{
    bool TextureFormatTraits::IsDepth(TextureFormat eFormat) noexcept
    {
        switch (eFormat)
        {
        case TextureFormat::Depth16:
        case TextureFormat::Depth24Stencil8:
        case TextureFormat::Depth32F:
            return true;
        case TextureFormat::Rgba8:
        case TextureFormat::Bgra8:
        case TextureFormat::R8:
        case TextureFormat::Rgba16F:
        case TextureFormat::Rgba32F:
            return false;
        }
        return false;
    }

    uint32_t TextureFormatTraits::GetBytesPerPixel(TextureFormat eFormat) noexcept
    {
        switch (eFormat)
        {
        case TextureFormat::R8:
            return 1;
        case TextureFormat::Depth16:
            return 2;
        case TextureFormat::Rgba8:
        case TextureFormat::Bgra8:
        case TextureFormat::Depth24Stencil8:
        case TextureFormat::Depth32F:
            return 4;
        case TextureFormat::Rgba16F:
            return 8;
        case TextureFormat::Rgba32F:
            return 16;
        }
        return 0;
    }

    bool TextureFormatTraits::HasStencil(TextureFormat eFormat) noexcept
    {
        return eFormat == TextureFormat::Depth24Stencil8;
    }

    std::string_view TextureFormatTraits::GetName(TextureFormat eFormat) noexcept
    {
        constexpr std::array<std::string_view, 8> kNames = {"Rgba8", "Bgra8", "R8", "Rgba16F", "Rgba32F", "Depth16", "Depth24Stencil8", "Depth32F"};
        const auto uIndex = static_cast<size_t>(eFormat);
        return uIndex < kNames.size() ? kNames[uIndex] : std::string_view("Unknown");
    }
}
