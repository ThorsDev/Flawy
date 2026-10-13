#pragma once

#include "Flawy/Platform/IPlatformSystem.hpp"

namespace Flawy::Platform
{
    class SdlPlatformSystem final : public IPlatformSystem
    {
    public:
        SdlPlatformSystem();
        ~SdlPlatformSystem() override;

        SdlPlatformSystem(const SdlPlatformSystem&) = delete;
        SdlPlatformSystem& operator=(const SdlPlatformSystem&) = delete;
        SdlPlatformSystem(SdlPlatformSystem&&) = delete;
        SdlPlatformSystem& operator=(SdlPlatformSystem&&) = delete;

        [[nodiscard]] std::unique_ptr<IWindow> OpenWindow(const WindowSpecification& oSpecification) override;
        void PumpEvents(IWindowEventHandler& oHandler) override;
    };
}
