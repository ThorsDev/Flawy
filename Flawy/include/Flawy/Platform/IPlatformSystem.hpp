#pragma once

#include "Flawy/Platform/IWindow.hpp"
#include "Flawy/Platform/IWindowEventHandler.hpp"
#include "Flawy/Platform/WindowSpecification.hpp"

#include <memory>

namespace Flawy::Platform
{
    class IPlatformSystem
    {
    public:
        virtual ~IPlatformSystem() = default;

        [[nodiscard]] virtual std::unique_ptr<IWindow> OpenWindow(const WindowSpecification& oSpecification) = 0;
        virtual void PumpEvents(IWindowEventHandler& oHandler) = 0;
    };
}
