#pragma once

#include "Flawy/Renderer/SamplerState.hpp"
#include "Flawy/Renderer/TextureFormat.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>

namespace Flawy::Renderer
{
    struct TextureDescriptor
    {
        uint32_t uWidth = 0;
        uint32_t uHeight = 0;
        TextureFormat eFormat = TextureFormat::Rgba8;
        SamplerState oSampler;
        bool bRenderTarget = false;
        std::string sDebugName;
    };

    class Texture final
    {
    public:
        struct Impl;

        Texture(const TextureDescriptor& oDescriptor, std::span<const std::byte> spanPixels);
        explicit Texture(const TextureDescriptor& oDescriptor);
        ~Texture();

        Texture(const Texture&) = delete;
        Texture& operator=(const Texture&) = delete;
        Texture(Texture&&) = delete;
        Texture& operator=(Texture&&) = delete;

        [[nodiscard]] uint32_t GetWidth() const noexcept;
        [[nodiscard]] uint32_t GetHeight() const noexcept;
        [[nodiscard]] TextureFormat GetFormat() const noexcept;
        [[nodiscard]] const SamplerState& GetSampler() const noexcept;
        [[nodiscard]] bool IsRenderTarget() const noexcept;
        [[nodiscard]] const Impl& GetImpl() const noexcept;

    private:
        std::unique_ptr<Impl> m_upImpl;
    };
}
