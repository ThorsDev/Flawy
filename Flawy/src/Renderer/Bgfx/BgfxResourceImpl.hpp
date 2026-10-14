#pragma once

#include "Flawy/Renderer/IndexBuffer.hpp"
#include "Flawy/Renderer/RenderTarget.hpp"
#include "Flawy/Renderer/SamplerState.hpp"
#include "Flawy/Renderer/Shader/ShaderInterface.hpp"
#include "Flawy/Renderer/ShaderParameters.hpp"
#include "Flawy/Renderer/ShaderProgram.hpp"
#include "Flawy/Renderer/Texture.hpp"
#include "Flawy/Renderer/TextureFormat.hpp"
#include "Flawy/Renderer/VertexBuffer.hpp"

#include <bgfx/bgfx.h>

#include <cstdint>
#include <memory>
#include <vector>

namespace Flawy::Renderer
{
    class BgfxHandleRelease final
    {
    public:
        BgfxHandleRelease() = delete;

        template <typename THandle>
        static void Destroy(THandle hHandle) noexcept
        {
            if (bgfx::isValid(hHandle))
            {
                bgfx::destroy(hHandle);
            }
        }
    };

    struct VertexBuffer::Impl
    {
        Impl() = default;

        ~Impl()
        {
            BgfxHandleRelease::Destroy(hStatic);
            BgfxHandleRelease::Destroy(hDynamic);
        }

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;

        bgfx::VertexBufferHandle hStatic = BGFX_INVALID_HANDLE;
        bgfx::DynamicVertexBufferHandle hDynamic = BGFX_INVALID_HANDLE;
        uint32_t uVertexCount = 0;
        uint32_t uStride = 0;
        BufferUsage eUsage = BufferUsage::Static;
    };

    struct IndexBuffer::Impl
    {
        Impl() = default;

        ~Impl()
        {
            BgfxHandleRelease::Destroy(hStatic);
            BgfxHandleRelease::Destroy(hDynamic);
        }

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;

        bgfx::IndexBufferHandle hStatic = BGFX_INVALID_HANDLE;
        bgfx::DynamicIndexBufferHandle hDynamic = BGFX_INVALID_HANDLE;
        uint32_t uIndexCount = 0;
        IndexFormat eFormat = IndexFormat::Uint16;
        BufferUsage eUsage = BufferUsage::Static;
    };

    struct Texture::Impl
    {
        Impl() = default;

        ~Impl()
        {
            BgfxHandleRelease::Destroy(hTexture);
        }

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;

        bgfx::TextureHandle hTexture = BGFX_INVALID_HANDLE;
        uint32_t uWidth = 0;
        uint32_t uHeight = 0;
        TextureFormat eFormat = TextureFormat::Rgba8;
        SamplerState oSampler;
        bool bRenderTarget = false;
    };

    struct RenderTarget::Impl
    {
        Impl() = default;
        ~Impl();

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;

        RenderTargetDescriptor oDescriptor;
        std::unique_ptr<Texture> upColor;
        std::unique_ptr<Texture> upDepth;
        bgfx::FrameBufferHandle hFrameBuffer = BGFX_INVALID_HANDLE;
    };

    struct ShaderProgram::Impl
    {
        Impl() = default;

        ~Impl()
        {
            BgfxHandleRelease::Destroy(hProgram);
            for (const bgfx::UniformHandle hUniform : vUniforms)
            {
                bgfx::destroy(hUniform);
            }
        }

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;

        bgfx::ProgramHandle hProgram = BGFX_INVALID_HANDLE;
        std::vector<bgfx::UniformHandle> vUniforms;
        std::vector<ShaderUniform> vInterface;
    };

    struct ShaderParameters::Impl
    {
        static constexpr uint32_t kNoSlot = 0xffffffffu;
        static constexpr uint32_t kTextureSamplerFlags = 0xffffffffu;

        struct ValueSlot
        {
            uint32_t uUniformIndex = 0;
            uint32_t uOffset = 0;
            bool bSet = false;
        };

        struct TextureSlot
        {
            uint32_t uUniformIndex = 0;
            uint8_t uStage = 0;
            const Texture* pTexture = nullptr;
            uint32_t uSamplerFlags = kTextureSamplerFlags;
        };

        explicit Impl(const ShaderProgram& oProgram);

        void Apply() const;
        [[nodiscard]] const ShaderUniform* FindUnboundTexture() const noexcept;

        const ShaderProgram* pProgram = nullptr;
        std::vector<float> vData;
        std::vector<ValueSlot> vValues;
        std::vector<TextureSlot> vTextures;
        std::vector<uint32_t> vSlotIndices;
    };
}
