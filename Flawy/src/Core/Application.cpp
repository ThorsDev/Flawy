#include "Flawy/Core/Application.hpp"

#include "Flawy/Core/Logger.hpp"
#include "Flawy/Platform/IPlatformSystem.hpp"
#include "Flawy/Platform/IWindow.hpp"
#include "Flawy/Platform/IWindowEventHandler.hpp"
#include "Flawy/Platform/PlatformSystemFactory.hpp"
#include "Flawy/Renderer/RenderDevice.hpp"
#include "Flawy/Renderer/RenderGraph/RenderGraph.hpp"

#include <chrono>
#include <stdexcept>
#include <string>
#include <utility>

namespace Flawy::Core
{
    struct Application::Impl
    {
        using Clock = std::chrono::steady_clock;

        class WindowEventRouter final : public Platform::IWindowEventHandler
        {
        public:
            explicit WindowEventRouter(Impl& oOwner) noexcept
                : m_oOwner(oOwner)
            {
            }

            void OnCloseRequested() override
            {
                m_oOwner.bRunning = false;
            }

            void OnResized(uint32_t uPixelWidth, uint32_t uPixelHeight) override
            {
                Renderer::RenderDevice& oRenderDevice = *m_oOwner.upRenderDevice;
                const bool bEmpty = uPixelWidth == 0 || uPixelHeight == 0;
                const bool bUnchanged = oRenderDevice.GetWidth() == uPixelWidth && oRenderDevice.GetHeight() == uPixelHeight;
                if (bEmpty || bUnchanged)
                {
                    return;
                }

                Logger::Get().info("Window resized to {}x{} pixels", uPixelWidth, uPixelHeight);
                oRenderDevice.Resize(uPixelWidth, uPixelHeight);
            }

        private:
            Impl& m_oOwner;
        };

        explicit Impl(ApplicationSpecification oSpec)
            : oSpecification(std::move(oSpec))
            , oEventRouter(*this)
        {
        }

        void RequireInitialized(const char* szOperation) const
        {
            if (!bInitialized)
            {
                throw std::logic_error(std::string("Application::") + szOperation + " called before Application::Initialize");
            }
        }

        [[nodiscard]] double AdvanceClock()
        {
            const Clock::time_point tNow = Clock::now();
            const std::chrono::duration<double> tDelta = tNow - tPreviousFrame;
            tPreviousFrame = tNow;
            return tDelta.count();
        }

        ApplicationSpecification oSpecification;
        bool bInitialized = false;
        bool bRunning = false;
        std::unique_ptr<Platform::IPlatformSystem> upPlatform;
        std::unique_ptr<Platform::IWindow> upWindow;
        std::unique_ptr<Renderer::RenderDevice> upRenderDevice;
        std::unique_ptr<Renderer::RenderGraph> upRenderGraph;
        WindowEventRouter oEventRouter;
        Clock::time_point tPreviousFrame;
    };

    Application::Application(ApplicationSpecification oSpecification)
        : m_upImpl(std::make_unique<Impl>(std::move(oSpecification)))
    {
        Logger::Initialize(m_upImpl->oSpecification.sName);
    }

    Application::~Application()
    {
        m_upImpl.reset();
        Logger::Shutdown();
    }

    int Application::Run()
    {
        Initialize();
        while (IsRunning())
        {
            Tick();
        }
        Shutdown();
        return 0;
    }

    void Application::Initialize()
    {
        if (m_upImpl->bInitialized)
        {
            return;
        }

        const ApplicationSpecification& oSpecification = m_upImpl->oSpecification;

        auto upPlatform = Platform::PlatformSystemFactory::Create();
        auto upWindow = upPlatform->OpenWindow(oSpecification.oWindow);

        Renderer::RenderDeviceSpecification oDeviceSpecification;
        oDeviceSpecification.oWindowHandle = upWindow->GetNativeHandle();
        oDeviceSpecification.uWidth = upWindow->GetPixelWidth();
        oDeviceSpecification.uHeight = upWindow->GetPixelHeight();
        oDeviceSpecification.eBackend = oSpecification.eRendererBackend;
        oDeviceSpecification.bVSync = oSpecification.bVSync;
        auto upRenderDevice = std::make_unique<Renderer::RenderDevice>(oDeviceSpecification);
        auto upRenderGraph = std::make_unique<Renderer::RenderGraph>(*upRenderDevice);

        m_upImpl->upPlatform = std::move(upPlatform);
        m_upImpl->upWindow = std::move(upWindow);
        m_upImpl->upRenderDevice = std::move(upRenderDevice);
        m_upImpl->upRenderGraph = std::move(upRenderGraph);
        m_upImpl->bInitialized = true;
        m_upImpl->bRunning = true;
        m_upImpl->tPreviousFrame = Impl::Clock::now();

        OnStart();
    }

    void Application::Tick()
    {
        m_upImpl->RequireInitialized("Tick");

        m_upImpl->upPlatform->PumpEvents(m_upImpl->oEventRouter);
        if (!m_upImpl->bRunning)
        {
            return;
        }

        OnUpdate(m_upImpl->AdvanceClock());

        Renderer::RenderGraph& oRenderGraph = *m_upImpl->upRenderGraph;
        oRenderGraph.BeginFrame();
        OnRender(oRenderGraph);
        oRenderGraph.Execute();
        m_upImpl->upRenderDevice->EndFrame();
    }

    void Application::Shutdown()
    {
        m_upImpl->RequireInitialized("Shutdown");

        OnShutdown();

        m_upImpl->upRenderGraph.reset();
        m_upImpl->upRenderDevice.reset();
        m_upImpl->upWindow.reset();
        m_upImpl->upPlatform.reset();
        m_upImpl->bRunning = false;
        m_upImpl->bInitialized = false;
    }

    const ApplicationSpecification& Application::GetSpecification() const noexcept
    {
        return m_upImpl->oSpecification;
    }

    bool Application::IsInitialized() const noexcept
    {
        return m_upImpl->bInitialized;
    }

    bool Application::IsRunning() const noexcept
    {
        return m_upImpl->bRunning;
    }

    Platform::IWindow& Application::GetWindow()
    {
        m_upImpl->RequireInitialized("GetWindow");
        return *m_upImpl->upWindow;
    }

    Renderer::RenderDevice& Application::GetRenderDevice()
    {
        m_upImpl->RequireInitialized("GetRenderDevice");
        return *m_upImpl->upRenderDevice;
    }

    Renderer::RenderGraph& Application::GetRenderGraph()
    {
        m_upImpl->RequireInitialized("GetRenderGraph");
        return *m_upImpl->upRenderGraph;
    }

    void Application::OnStart()
    {
    }

    void Application::OnUpdate(double)
    {
    }

    void Application::OnRender(Renderer::RenderGraph&)
    {
    }

    void Application::OnShutdown()
    {
    }

    void Application::RequestExit() noexcept
    {
        m_upImpl->bRunning = false;
    }
}
