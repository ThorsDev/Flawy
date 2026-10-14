#pragma once

#include "Flawy/Renderer/RenderDeviceSpecification.hpp"
#include "Flawy/Renderer/RendererBackend.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <string_view>

namespace Flawy::Renderer
{
    class RenderDevice final
    {
    public:
        explicit RenderDevice(const RenderDeviceSpecification& oSpecification);
        ~RenderDevice();

        RenderDevice(const RenderDevice&) = delete;
        RenderDevice& operator=(const RenderDevice&) = delete;
        RenderDevice(RenderDevice&&) = delete;
        RenderDevice& operator=(RenderDevice&&) = delete;

        void Resize(uint32_t uWidth, uint32_t uHeight);
        void EndFrame();
        void RequestScreenshot(const std::filesystem::path& oPath);

        [[nodiscard]] RendererBackend GetBackend() const noexcept;
        [[nodiscard]] std::string_view GetBackendName() const noexcept;
        [[nodiscard]] uint32_t GetWidth() const noexcept;
        [[nodiscard]] uint32_t GetHeight() const noexcept;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_upImpl;
    };
}
