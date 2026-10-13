#include "Platform/Sdl/SdlWindow.hpp"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_properties.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace Flawy::Platform
{
    namespace
    {
        constexpr std::string_view kWindowsDriver = "windows";
        constexpr std::string_view kCocoaDriver = "cocoa";
        constexpr std::string_view kX11Driver = "x11";
        constexpr std::string_view kWaylandDriver = "wayland";

        SDL_WindowFlags BuildWindowFlags(const WindowSpecification& oSpecification) noexcept
        {
            SDL_WindowFlags uFlags = SDL_WINDOW_HIGH_PIXEL_DENSITY;
            if (oSpecification.bResizable)
            {
                uFlags |= SDL_WINDOW_RESIZABLE;
            }
            return uFlags;
        }

        uint32_t ToUnsigned(int iValue) noexcept
        {
            return iValue > 0 ? static_cast<uint32_t>(iValue) : 0u;
        }

        void* ToPointer(Sint64 iValue) noexcept
        {
            return reinterpret_cast<void*>(static_cast<uintptr_t>(iValue));
        }
    }

    void SdlWindow::WindowDeleter::operator()(SDL_Window* pWindow) const noexcept
    {
        SDL_DestroyWindow(pWindow);
    }

    SdlWindow::SdlWindow(const WindowSpecification& oSpecification)
        : m_upWindow(SDL_CreateWindow(oSpecification.sTitle.c_str(),
                                      static_cast<int>(oSpecification.uWidth),
                                      static_cast<int>(oSpecification.uHeight),
                                      BuildWindowFlags(oSpecification)))
    {
        if (!m_upWindow)
        {
            throw std::runtime_error(SDL_GetError());
        }
    }

    uint32_t SdlWindow::GetWidth() const noexcept
    {
        return QuerySize().uWidth;
    }

    uint32_t SdlWindow::GetHeight() const noexcept
    {
        return QuerySize().uHeight;
    }

    uint32_t SdlWindow::GetPixelWidth() const noexcept
    {
        return QueryPixelSize().uWidth;
    }

    uint32_t SdlWindow::GetPixelHeight() const noexcept
    {
        return QueryPixelSize().uHeight;
    }

    NativeWindowHandle SdlWindow::GetNativeHandle() const
    {
        const SDL_PropertiesID hProperties = SDL_GetWindowProperties(m_upWindow.get());
        const char* szDriver = SDL_GetCurrentVideoDriver();
        const std::string_view svDriver = szDriver != nullptr ? szDriver : "";

        NativeWindowHandle oHandle;
        if (svDriver == kWindowsDriver)
        {
            oHandle.pWindow = SDL_GetPointerProperty(hProperties, SDL_PROP_WINDOW_WIN32_HWND_POINTER, nullptr);
            oHandle.eSystem = NativeWindowSystem::Win32;
        }
        else if (svDriver == kCocoaDriver)
        {
            oHandle.pWindow = SDL_GetPointerProperty(hProperties, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, nullptr);
            oHandle.eSystem = NativeWindowSystem::Cocoa;
        }
        else if (svDriver == kX11Driver)
        {
            oHandle.pWindow = ToPointer(SDL_GetNumberProperty(hProperties, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0));
            oHandle.pDisplay = SDL_GetPointerProperty(hProperties, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, nullptr);
            oHandle.eSystem = NativeWindowSystem::X11;
        }
        else if (svDriver == kWaylandDriver)
        {
            oHandle.pWindow = SDL_GetPointerProperty(hProperties, SDL_PROP_WINDOW_WAYLAND_SURFACE_POINTER, nullptr);
            oHandle.pDisplay = SDL_GetPointerProperty(hProperties, SDL_PROP_WINDOW_WAYLAND_DISPLAY_POINTER, nullptr);
            oHandle.eSystem = NativeWindowSystem::Wayland;
        }
        else
        {
            throw std::runtime_error("Unsupported SDL video driver: " + std::string(svDriver));
        }

        if (oHandle.pWindow == nullptr)
        {
            throw std::runtime_error("Failed to query the native window handle: " + std::string(SDL_GetError()));
        }

        return oHandle;
    }

    void SdlWindow::SetTitle(std::string_view svTitle)
    {
        const std::string sTitle(svTitle);
        SDL_SetWindowTitle(m_upWindow.get(), sTitle.c_str());
    }

    SdlWindow::Extent SdlWindow::QuerySize() const noexcept
    {
        int iWidth = 0;
        int iHeight = 0;
        SDL_GetWindowSize(m_upWindow.get(), &iWidth, &iHeight);
        return {ToUnsigned(iWidth), ToUnsigned(iHeight)};
    }

    SdlWindow::Extent SdlWindow::QueryPixelSize() const noexcept
    {
        int iWidth = 0;
        int iHeight = 0;
        SDL_GetWindowSizeInPixels(m_upWindow.get(), &iWidth, &iHeight);
        return {ToUnsigned(iWidth), ToUnsigned(iHeight)};
    }
}
