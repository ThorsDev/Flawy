#include "Flawy/Renderer/RenderDevice.hpp"

#include "Renderer/Bgfx/BgfxCallbackAdapter.hpp"
#include "Renderer/Bgfx/BgfxRendererBackendMapping.hpp"

#include <bgfx/bgfx.h>

#include <stdexcept>
#include <string>

namespace Flawy::Renderer
{
    namespace
    {
        bgfx::NativeWindowHandleType::Enum ToNativeWindowHandleType(Platform::NativeWindowSystem eSystem) noexcept
        {
            return eSystem == Platform::NativeWindowSystem::Wayland ? bgfx::NativeWindowHandleType::Wayland : bgfx::NativeWindowHandleType::Default;
        }
    }

    struct RenderDevice::Impl
    {
        explicit Impl(const RenderDeviceSpecification& oSpecification)
            : uResetFlags(oSpecification.bVSync ? BGFX_RESET_VSYNC : BGFX_RESET_NONE)
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
        }

        ~Impl()
        {
            bgfx::shutdown();
        }

        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;
        Impl(Impl&&) = delete;
        Impl& operator=(Impl&&) = delete;

        BgfxCallbackAdapter oCallback;
        uint32_t uResetFlags = BGFX_RESET_NONE;
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

    void RenderDevice::EndFrame()
    {
        bgfx::frame();
    }

    void RenderDevice::RequestScreenshot(const std::filesystem::path& oPath)
    {
        const std::u8string sPath = oPath.u8string();
        bgfx::requestScreenShot(BGFX_INVALID_HANDLE, reinterpret_cast<const char*>(sPath.c_str()));
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
