#pragma once

#include <cstdint>

namespace Flawy::Renderer
{
    enum class DepthTest : uint8_t
    {
        Disabled,
        Less,
        LessEqual,
        Always
    };

    enum class CullMode : uint8_t
    {
        None,
        Back,
        Front
    };

    enum class BlendMode : uint8_t
    {
        Opaque,
        Alpha,
        Additive
    };

    struct RenderState
    {
        DepthTest eDepthTest = DepthTest::Less;
        bool bDepthWrite = true;
        CullMode eCull = CullMode::Back;
        BlendMode eBlend = BlendMode::Opaque;
        bool bColorWrite = true;
    };
}
