#pragma once

#include <cstdint>

namespace Flawy::Platform
{
    enum class NativeWindowSystem : uint8_t
    {
        Unknown,
        Win32,
        Cocoa,
        X11,
        Wayland
    };

    struct NativeWindowHandle
    {
        void* pWindow = nullptr;
        void* pDisplay = nullptr;
        NativeWindowSystem eSystem = NativeWindowSystem::Unknown;
    };
}
