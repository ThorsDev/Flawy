#pragma once

#include "Flawy/Platform/IWindow.hpp"
#include "Flawy/Platform/WindowSpecification.hpp"

#include <SDL3/SDL_video.h>

#include <memory>

namespace Flawy::Platform
{
    class SdlWindow final : public IWindow
    {
    public:
        explicit SdlWindow(const WindowSpecification& oSpecification);

        [[nodiscard]] uint32_t GetWidth() const noexcept override;
        [[nodiscard]] uint32_t GetHeight() const noexcept override;
        [[nodiscard]] uint32_t GetPixelWidth() const noexcept override;
        [[nodiscard]] uint32_t GetPixelHeight() const noexcept override;
        [[nodiscard]] NativeWindowHandle GetNativeHandle() const override;

        void SetTitle(std::string_view svTitle) override;

    private:
        struct WindowDeleter
        {
            void operator()(SDL_Window* pWindow) const noexcept;
        };

        struct Extent
        {
            uint32_t uWidth = 0;
            uint32_t uHeight = 0;
        };

        [[nodiscard]] Extent QuerySize() const noexcept;
        [[nodiscard]] Extent QueryPixelSize() const noexcept;

        std::unique_ptr<SDL_Window, WindowDeleter> m_upWindow;
    };
}
