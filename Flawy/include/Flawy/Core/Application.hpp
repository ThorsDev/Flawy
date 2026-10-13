#pragma once

#include "Flawy/Platform/WindowSpecification.hpp"
#include "Flawy/Renderer/RendererBackend.hpp"

#include <memory>
#include <string>

namespace Flawy::Platform
{
    class IWindow;
}

namespace Flawy::Renderer
{
    class RenderDevice;
}

namespace Flawy::Core
{
    struct ApplicationSpecification
    {
        std::string sName = "Flawy Application";
        Platform::WindowSpecification oWindow;
        Renderer::RendererBackend eRendererBackend = Renderer::RendererBackend::Default;
        bool bVSync = true;
    };

    class Application
    {
    public:
        explicit Application(ApplicationSpecification oSpecification);
        virtual ~Application();

        Application(const Application&) = delete;
        Application& operator=(const Application&) = delete;
        Application(Application&&) = delete;
        Application& operator=(Application&&) = delete;

        int Run();
        void Initialize();
        void Tick();
        void Shutdown();

        [[nodiscard]] const ApplicationSpecification& GetSpecification() const noexcept;
        [[nodiscard]] bool IsInitialized() const noexcept;
        [[nodiscard]] bool IsRunning() const noexcept;
        [[nodiscard]] Platform::IWindow& GetWindow();
        [[nodiscard]] Renderer::RenderDevice& GetRenderDevice();

    protected:
        virtual void OnStart();
        virtual void OnUpdate(double dDeltaSeconds);
        virtual void OnRender();
        virtual void OnShutdown();

        void RequestExit() noexcept;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_upImpl;
    };
}
