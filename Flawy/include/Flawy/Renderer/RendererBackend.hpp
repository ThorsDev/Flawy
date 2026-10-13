#pragma once

#include <cstdint>

namespace Flawy::Renderer
{
    enum class RendererBackend : uint8_t
    {
        Default,
        Direct3D11,
        Vulkan
    };
}
