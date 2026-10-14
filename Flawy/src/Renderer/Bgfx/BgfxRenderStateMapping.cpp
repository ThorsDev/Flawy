#include "Renderer/Bgfx/BgfxRenderStateMapping.hpp"

namespace Flawy::Renderer
{
    namespace
    {
        uint64_t ToDepthTestState(DepthTest eDepthTest) noexcept
        {
            switch (eDepthTest)
            {
            case DepthTest::Disabled:
                return 0;
            case DepthTest::Less:
                return BGFX_STATE_DEPTH_TEST_LESS;
            case DepthTest::LessEqual:
                return BGFX_STATE_DEPTH_TEST_LEQUAL;
            case DepthTest::Always:
                return BGFX_STATE_DEPTH_TEST_ALWAYS;
            }
            return 0;
        }

        uint64_t ToCullState(CullMode eCull) noexcept
        {
            switch (eCull)
            {
            case CullMode::None:
                return 0;
            case CullMode::Back:
                return BGFX_STATE_CULL_CW;
            case CullMode::Front:
                return BGFX_STATE_CULL_CCW;
            }
            return 0;
        }

        uint64_t ToBlendState(BlendMode eBlend) noexcept
        {
            switch (eBlend)
            {
            case BlendMode::Opaque:
                return 0;
            case BlendMode::Alpha:
                return BGFX_STATE_BLEND_ALPHA;
            case BlendMode::Additive:
                return BGFX_STATE_BLEND_ADD;
            }
            return 0;
        }

        uint32_t ToWrapFlags(TextureWrap eWrap, uint32_t uClampFlag, uint32_t uMirrorFlag) noexcept
        {
            switch (eWrap)
            {
            case TextureWrap::Repeat:
                return 0;
            case TextureWrap::Clamp:
                return uClampFlag;
            case TextureWrap::Mirror:
                return uMirrorFlag;
            }
            return 0;
        }
    }

    uint64_t BgfxRenderStateMapping::ToBgfxState(const RenderState& oState) noexcept
    {
        uint64_t uState = ToDepthTestState(oState.eDepthTest) | ToCullState(oState.eCull) | ToBlendState(oState.eBlend);
        if (oState.bDepthWrite)
        {
            uState |= BGFX_STATE_WRITE_Z;
        }
        if (oState.bColorWrite)
        {
            uState |= BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A;
        }
        return uState;
    }

    uint32_t BgfxRenderStateMapping::ToBgfxSamplerFlags(const SamplerState& oSampler) noexcept
    {
        uint32_t uFlags = BGFX_SAMPLER_NONE;
        if (oSampler.eFilter == TextureFilter::Point)
        {
            uFlags |= BGFX_SAMPLER_MIN_POINT | BGFX_SAMPLER_MAG_POINT | BGFX_SAMPLER_MIP_POINT;
        }
        uFlags |= ToWrapFlags(oSampler.eWrapU, BGFX_SAMPLER_U_CLAMP, BGFX_SAMPLER_U_MIRROR);
        uFlags |= ToWrapFlags(oSampler.eWrapV, BGFX_SAMPLER_V_CLAMP, BGFX_SAMPLER_V_MIRROR);
        return uFlags;
    }

    bgfx::TextureFormat::Enum BgfxRenderStateMapping::ToBgfxTextureFormat(TextureFormat eFormat) noexcept
    {
        switch (eFormat)
        {
        case TextureFormat::Rgba8:
            return bgfx::TextureFormat::RGBA8;
        case TextureFormat::Bgra8:
            return bgfx::TextureFormat::BGRA8;
        case TextureFormat::R8:
            return bgfx::TextureFormat::R8;
        case TextureFormat::Rgba16F:
            return bgfx::TextureFormat::RGBA16F;
        case TextureFormat::Rgba32F:
            return bgfx::TextureFormat::RGBA32F;
        case TextureFormat::Depth16:
            return bgfx::TextureFormat::D16;
        case TextureFormat::Depth24Stencil8:
            return bgfx::TextureFormat::D24S8;
        case TextureFormat::Depth32F:
            return bgfx::TextureFormat::D32F;
        }
        return bgfx::TextureFormat::Unknown;
    }
}
