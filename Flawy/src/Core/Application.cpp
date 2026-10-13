#include "Flawy/Core/Application.hpp"

#include "Flawy/Core/Logger.hpp"

#include <chrono>
#include <utility>

namespace Flawy::Core
{
    Application::Application(ApplicationSpecification oSpecification)
        : m_oSpecification(std::move(oSpecification))
    {
        Logger::Initialize(m_oSpecification.sName);
    }

    Application::~Application()
    {
        Logger::Shutdown();
    }

    int Application::Run()
    {
        using Clock = std::chrono::steady_clock;

        m_bRunning = true;
        OnStart();

        auto tPreviousFrame = Clock::now();
        while (m_bRunning)
        {
            const auto tCurrentFrame = Clock::now();
            const std::chrono::duration<double> tDelta = tCurrentFrame - tPreviousFrame;
            tPreviousFrame = tCurrentFrame;

            OnUpdate(tDelta.count());
        }

        OnShutdown();
        return 0;
    }

    const ApplicationSpecification& Application::GetSpecification() const noexcept
    {
        return m_oSpecification;
    }

    bool Application::IsRunning() const noexcept
    {
        return m_bRunning;
    }

    void Application::OnStart()
    {
    }

    void Application::OnUpdate(double)
    {
    }

    void Application::OnShutdown()
    {
    }

    void Application::RequestExit() noexcept
    {
        m_bRunning = false;
    }
}
