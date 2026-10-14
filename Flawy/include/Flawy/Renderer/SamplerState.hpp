#pragma once

#include <cstdint>

namespace Flawy::Renderer
{
    enum class TextureFilter : uint8_t
    {
        Linear,
        Point
    };

    enum class TextureWrap : uint8_t
    {
        Repeat,
        Clamp,
        Mirror
    };

    struct SamplerState
    {
        TextureFilter eFilter = TextureFilter::Linear;
        TextureWrap eWrapU = TextureWrap::Repeat;
        TextureWrap eWrapV = TextureWrap::Repeat;

        [[nodiscard]] bool operator==(const SamplerState&) const noexcept = default;
    };
}
