#pragma once

#include <cstdint>

namespace Flawy::Platform
{
    class IWindowEventHandler
    {
    public:
        virtual ~IWindowEventHandler() = default;

        virtual void OnCloseRequested() = 0;
        virtual void OnResized(uint32_t uPixelWidth, uint32_t uPixelHeight) = 0;
    };
}
