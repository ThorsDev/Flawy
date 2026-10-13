#include "Flawy/Renderer/RenderDevice.hpp"

#include "Flawy/Renderer/ShaderProgram.hpp"
#include "Flawy/Renderer/VertexBuffer.hpp"
#include "Renderer/Bgfx/BgfxCallbackAdapter.hpp"
#include "Renderer/Bgfx/BgfxRendererBackendMapping.hpp"
#include "Renderer/Bgfx/BgfxResourceImpl.hpp"

#include <bgfx/bgfx.h>
#include <glm/gtc/type_ptr.hpp>

#include <stdexcept>
#include <string>

namespace Flawy::Renderer
{
    namespace
    {
        constexpr bgfx::ViewId kMainViewId = 0;
        constexpr uint16_t kMainViewClearFlags = BGFX_CLEAR_COLOR | BGFX_CLEAR_DEPTH;
        constexpr float kClearDepth = 1.0f;
        constexpr uint8_t kClearStencil = 0;
        constexpr uint64_t kOpaqueDrawState = BGFX_STATE_WRITE_RGB | BGFX_STATE_WRITE_A | BGFX_STATE_WRITE_Z | BGFX_STATE_DEPTH_TEST_LESS;

        bgfx::NativeWindowHandleType::Enum ToNativeWindowHandleType(Platform::NativeWindowSystem eSystem) noexcept
        {
            return eSystem == Platform::NativeWindowSystem::Wayland ? bgfx::NativeWindowHandleType::Wayland : bgfx::NativeWindowHandleType::Default;
        }
    }

    struct RenderDevice::Impl
    {
        explicit Impl(const RenderDeviceSpecification& oSpecification)
            : uResetFlags(oSpecification.bVSync ? BGFX_RESET_VSYNC : BGFX_RESET_NONE)
            , uClearColorRgba(oSpecification.uClearColorRgba)
            , uWidth(oSpecification.uWidth)
            , uHeight(oSpecification.uHeight)
        {
            const bgfx::RendererType::Enum eRendererType = BgfxRendererBackendMapping::ResolveRendererType(oSpecification.eBackend);
            eBackend = BgfxRendererBackendMapping::ToRendererBackend(eRendererType);

            bgfx::renderFrame();

            bgfx::Init oInit;
            oInit.type = eRendererType;
            oInit.fallback = false;
            oInit.debug = oSpecification.bDebug;
            oInit.platformData.type = ToNativeWindowHandleType(oSpecification.oWindowHandle.eSystem);
            oInit.swapChain.nwh = oSpecification.oWindowHandle.pWindow;
            oInit.swapChain.ndt = oSpecification.oWindowHandle.pDisplay;
            oInit.swapChain.width = uWidth;
            oInit.swapChain.height = uHeight;
            oInit.reset = uResetFlags;
            oInit.callback = &oCallback;

            if (!bgfx::init(oInit))
            {
                throw std::runtime_error("Failed to initialize bgfx with renderer " + std::string(bgfx::getRendererName(eRendererType)));
            }

            svBackendName = bgfx::getRendererName(bgfx::getRendererType());
            ApplyClearColor();
        }

        ~Impl()
        {
            bgfx::shutdown();
        }

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;

        void ApplyClearColor() const noexcept
        {
            bgfx::setViewClear(kMainViewId, kMainViewClearFlags, uClearColorRgba, kClearDepth, kClearStencil);
        }

        BgfxCallbackAdapter oCallback;
        uint32_t uResetFlags = BGFX_RESET_NONE;
        uint32_t uClearColorRgba = 0;
        uint32_t uWidth = 0;
        uint32_t uHeight = 0;
        RendererBackend eBackend = RendererBackend::Default;
        std::string_view svBackendName;
    };

    RenderDevice::RenderDevice(const RenderDeviceSpecification& oSpecification)
        : m_upImpl(std::make_unique<Impl>(oSpecification))
    {
    }

    RenderDevice::~RenderDevice() = default;

    void RenderDevice::Resize(uint32_t uWidth, uint32_t uHeight)
    {
        if (uWidth == 0 || uHeight == 0)
        {
            return;
        }

        m_upImpl->uWidth = uWidth;
        m_upImpl->uHeight = uHeight;

        bgfx::SwapChain oSwapChain;
        oSwapChain.width = uWidth;
        oSwapChain.height = uHeight;
        bgfx::reset(m_upImpl->uResetFlags, &oSwapChain);
    }

    void RenderDevice::SetClearColor(uint32_t uRgba) noexcept
    {
        m_upImpl->uClearColorRgba = uRgba;
        m_upImpl->ApplyClearColor();
    }

    void RenderDevice::BeginFrame()
    {
        bgfx::setViewRect(kMainViewId, 0, 0, static_cast<uint16_t>(m_upImpl->uWidth), static_cast<uint16_t>(m_upImpl->uHeight));
        bgfx::touch(kMainViewId);
    }

    void RenderDevice::EndFrame()
    {
        bgfx::frame();
    }

    void RenderDevice::SetViewTransform(ViewId uViewId, const glm::mat4& matView, const glm::mat4& matProjection)
    {
        bgfx::setViewTransform(uViewId, glm::value_ptr(matView), glm::value_ptr(matProjection));
    }

    void RenderDevice::Submit(ViewId uViewId, const DrawCommand& oCommand)
    {
        if (oCommand.pVertexBuffer == nullptr || oCommand.pProgram == nullptr)
        {
            throw std::invalid_argument("DrawCommand requires a vertex buffer and a shader program");
        }

        bgfx::setTransform(glm::value_ptr(oCommand.matModel));
        bgfx::setVertexBuffer(0, oCommand.pVertexBuffer->GetImpl().hBuffer);
        bgfx::setState(kOpaqueDrawState);
        bgfx::submit(uViewId, oCommand.pProgram->GetImpl().hProgram);
    }

    RendererBackend RenderDevice::GetBackend() const noexcept
    {
        return m_upImpl->eBackend;
    }

    std::string_view RenderDevice::GetBackendName() const noexcept
    {
        return m_upImpl->svBackendName;
    }

    uint32_t RenderDevice::GetWidth() const noexcept
    {
        return m_upImpl->uWidth;
    }

    uint32_t RenderDevice::GetHeight() const noexcept
    {
        return m_upImpl->uHeight;
    }
}
