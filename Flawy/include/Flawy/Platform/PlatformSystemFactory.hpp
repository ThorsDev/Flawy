#pragma once

#include "Flawy/Platform/IPlatformSystem.hpp"

#include <memory>

namespace Flawy::Platform
{
    class PlatformSystemFactory final
    {
    public:
        PlatformSystemFactory() = delete;

        [[nodiscard]] static std::unique_ptr<IPlatformSystem> Create();
    };
}
