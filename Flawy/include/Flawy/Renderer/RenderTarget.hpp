#pragma once

#include "Flawy/Renderer/SamplerState.hpp"
#include "Flawy/Renderer/TextureFormat.hpp"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

namespace Flawy::Renderer
{
    class Texture;

    struct RenderTargetDescriptor
    {
        uint32_t uWidth = 0;
        uint32_t uHeight = 0;
        TextureFormat eColorFormat = TextureFormat::Rgba8;
        std::optional<TextureFormat> optDepthFormat = TextureFormat::Depth24Stencil8;
        SamplerState oSampler{TextureFilter::Linear, TextureWrap::Clamp, TextureWrap::Clamp};
        std::string sDebugName;

        [[nodiscard]] bool operator==(const RenderTargetDescriptor&) const noexcept = default;
    };

    class RenderTarget final
    {
    public:
        struct Impl;

        explicit RenderTarget(const RenderTargetDescriptor& oDescriptor);
        ~RenderTarget();

        RenderTarget(const RenderTarget&) = delete;
        RenderTarget& operator=(const RenderTarget&) = delete;
        RenderTarget(RenderTarget&&) = delete;
        RenderTarget& operator=(RenderTarget&&) = delete;

        [[nodiscard]] const RenderTargetDescriptor& GetDescriptor() const noexcept;
        [[nodiscard]] uint32_t GetWidth() const noexcept;
        [[nodiscard]] uint32_t GetHeight() const noexcept;
        [[nodiscard]] const Texture& GetColorTexture() const noexcept;
        [[nodiscard]] bool HasDepth() const noexcept;
        [[nodiscard]] const Impl& GetImpl() const noexcept;

    private:
        std::unique_ptr<Impl> m_upImpl;
    };
}
