#include "SandboxApplication.hpp"

namespace Sandbox
{
    SandboxApplication::SandboxApplication()
        : Application({.sName = "Flawy-Sandbox"})
    {
    }

    void SandboxApplication::OnStart()
    {
        auto& oLogger = Flawy::Core::Logger::Get();
        oLogger.info("Flawy Engine {}", Flawy::Core::BuildInfo::GetEngineVersion());

        for (const auto& oDependency : Flawy::Core::BuildInfo::GetDependencyVersions())
        {
            oLogger.info("  {:<14} {}", oDependency.sName, oDependency.sVersion);
        }

        RequestExit();
    }

    void SandboxApplication::OnShutdown()
    {
        Flawy::Core::Logger::Get().info("Sandbox shut down");
    }
}
