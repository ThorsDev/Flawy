#pragma once

#include "Flawy/Renderer/RenderState.hpp"
#include "Flawy/Renderer/SamplerState.hpp"
#include "Flawy/Renderer/TextureFormat.hpp"

#include <bgfx/bgfx.h>

#include <cstdint>

namespace Flawy::Renderer
{
    class BgfxRenderStateMapping final
    {
    public:
        BgfxRenderStateMapping() = delete;

        [[nodiscard]] static uint64_t ToBgfxState(const RenderState& oState) noexcept;
        [[nodiscard]] static uint32_t ToBgfxSamplerFlags(const SamplerState& oSampler) noexcept;
        [[nodiscard]] static bgfx::TextureFormat::Enum ToBgfxTextureFormat(TextureFormat eFormat) noexcept;
    };
}
