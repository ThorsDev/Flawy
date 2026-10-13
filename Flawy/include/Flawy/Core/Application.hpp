#pragma once

#include <string>

namespace Flawy::Core
{
    struct ApplicationSpecification
    {
        std::string sName = "Flawy Application";
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

        [[nodiscard]] const ApplicationSpecification& GetSpecification() const noexcept;
        [[nodiscard]] bool IsRunning() const noexcept;

    protected:
        virtual void OnStart();
        virtual void OnUpdate(double dDeltaSeconds);
        virtual void OnShutdown();

        void RequestExit() noexcept;

    private:
        ApplicationSpecification m_oSpecification;
        bool m_bRunning = false;
    };
}
