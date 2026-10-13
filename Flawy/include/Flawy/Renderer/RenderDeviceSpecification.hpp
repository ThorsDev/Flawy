#pragma once

#include "Flawy/Platform/NativeWindowHandle.hpp"
#include "Flawy/Renderer/RendererBackend.hpp"

#include <cstdint>

namespace Flawy::Renderer
{
    struct RenderDeviceSpecification
    {
        Platform::NativeWindowHandle oWindowHandle;
        uint32_t uWidth = 0;
        uint32_t uHeight = 0;
        RendererBackend eBackend = RendererBackend::Default;
        bool bVSync = true;
        bool bDebug = false;
        uint32_t uClearColorRgba = 0x303030ff;
    };
}
