#include "Flawy/Platform/PlatformSystemFactory.hpp"

#include "Platform/Sdl/SdlPlatformSystem.hpp"

namespace Flawy::Platform
{
    std::unique_ptr<IPlatformSystem> PlatformSystemFactory::Create()
    {
        return std::make_unique<SdlPlatformSystem>();
    }
}
