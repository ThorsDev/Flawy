#pragma once

#include "Flawy/Platform/NativeWindowHandle.hpp"

#include <cstdint>
#include <string_view>

namespace Flawy::Platform
{
    class IWindow
    {
    public:
        virtual ~IWindow() = default;

        [[nodiscard]] virtual uint32_t GetWidth() const noexcept = 0;
        [[nodiscard]] virtual uint32_t GetHeight() const noexcept = 0;
        [[nodiscard]] virtual uint32_t GetPixelWidth() const noexcept = 0;
        [[nodiscard]] virtual uint32_t GetPixelHeight() const noexcept = 0;
        [[nodiscard]] virtual NativeWindowHandle GetNativeHandle() const = 0;

        virtual void SetTitle(std::string_view svTitle) = 0;
    };
}
