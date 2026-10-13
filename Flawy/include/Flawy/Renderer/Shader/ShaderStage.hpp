#pragma once

#include <cstdint>

namespace Flawy::Renderer
{
    enum class ShaderStage : uint8_t
    {
        Vertex,
        Fragment,
        Compute
    };
}
