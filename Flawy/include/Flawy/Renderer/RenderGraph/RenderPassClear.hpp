#pragma once

#include <cstdint>

namespace Flawy::Renderer
{
    struct RenderPassClear
    {
        bool bClearColor = true;
        uint32_t uColorRgba = 0x000000ffu;
        bool bClearDepth = true;
        float fDepth = 1.0f;
        bool bClearStencil = false;
        uint8_t uStencil = 0;

        [[nodiscard]] static RenderPassClear None() noexcept
        {
            return {false, 0x000000ffu, false, 1.0f, false, 0};
        }
    };
}
