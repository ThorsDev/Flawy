#include "Platform/Sdl/SdlPlatformSystem.hpp"

#include "Platform/Sdl/SdlWindow.hpp"

#include <SDL3/SDL.h>

#include <stdexcept>

namespace Flawy::Platform
{
    SdlPlatformSystem::SdlPlatformSystem()
    {
        if (!SDL_Init(SDL_INIT_VIDEO))
        {
            throw std::runtime_error(SDL_GetError());
        }
    }

    SdlPlatformSystem::~SdlPlatformSystem()
    {
        SDL_Quit();
    }

    std::unique_ptr<IWindow> SdlPlatformSystem::OpenWindow(const WindowSpecification& oSpecification)
    {
        return std::make_unique<SdlWindow>(oSpecification);
    }

    void SdlPlatformSystem::PumpEvents(IWindowEventHandler& oHandler)
    {
        SDL_Event oEvent;
        while (SDL_PollEvent(&oEvent))
        {
            switch (oEvent.type)
            {
            case SDL_EVENT_QUIT:
            case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
                oHandler.OnCloseRequested();
                break;
            case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
                oHandler.OnResized(static_cast<uint32_t>(oEvent.window.data1), static_cast<uint32_t>(oEvent.window.data2));
                break;
            default:
                break;
            }
        }
    }
}
