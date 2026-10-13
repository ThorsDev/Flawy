#pragma once

#include "Flawy/Renderer/DrawCommand.hpp"
#include "Flawy/Renderer/RenderDeviceSpecification.hpp"
#include "Flawy/Renderer/RendererBackend.hpp"

#include <glm/mat4x4.hpp>

#include <cstdint>
#include <memory>
#include <string_view>

namespace Flawy::Renderer
{
    using ViewId = uint16_t;

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
        void SetClearColor(uint32_t uRgba) noexcept;
        void BeginFrame();
        void EndFrame();
        void SetViewTransform(ViewId uViewId, const glm::mat4& matView, const glm::mat4& matProjection);
        void Submit(ViewId uViewId, const DrawCommand& oCommand);

        [[nodiscard]] RendererBackend GetBackend() const noexcept;
        [[nodiscard]] std::string_view GetBackendName() const noexcept;
        [[nodiscard]] uint32_t GetWidth() const noexcept;
        [[nodiscard]] uint32_t GetHeight() const noexcept;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_upImpl;
    };
}
