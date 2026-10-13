#pragma once

#include <Flawy/Flawy.hpp>

namespace Sandbox
{
    class SandboxApplication final : public Flawy::Core::Application
    {
    public:
        SandboxApplication();

    protected:
        void OnStart() override;
        void OnShutdown() override;
    };
}
