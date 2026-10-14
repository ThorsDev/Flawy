#pragma once

#include <cstdint>
#include <string_view>

namespace Flawy::Renderer
{
    enum class TextureFormat : uint8_t
    {
        Rgba8,
        Bgra8,
        R8,
        Rgba16F,
        Rgba32F,
        Depth16,
        Depth24Stencil8,
        Depth32F
    };

    class TextureFormatTraits final
    {
    public:
        TextureFormatTraits() = delete;

        [[nodiscard]] static bool IsDepth(TextureFormat eFormat) noexcept;
        [[nodiscard]] static uint32_t GetBytesPerPixel(TextureFormat eFormat) noexcept;
        [[nodiscard]] static bool HasStencil(TextureFormat eFormat) noexcept;
        [[nodiscard]] static std::string_view GetName(TextureFormat eFormat) noexcept;
    };
}
