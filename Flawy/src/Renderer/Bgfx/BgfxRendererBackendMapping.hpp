#pragma once

#include "Flawy/Renderer/RendererBackend.hpp"

#include <bgfx/bgfx.h>

namespace Flawy::Renderer
{
    class BgfxRendererBackendMapping final
    {
    public:
        BgfxRendererBackendMapping() = delete;

        [[nodiscard]] static bgfx::RendererType::Enum ResolveRendererType(RendererBackend eBackend);
        [[nodiscard]] static RendererBackend ToRendererBackend(bgfx::RendererType::Enum eRendererType);
    };
}
