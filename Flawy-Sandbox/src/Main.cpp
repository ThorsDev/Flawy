#include "SandboxApplication.hpp"

#include <cstddef>
#include <exception>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
    constexpr std::string_view kRendererOption = "--renderer=";

    Flawy::Renderer::RendererBackend ParseRendererName(std::string_view svName)
    {
        if (svName == "d3d11")
        {
            return Flawy::Renderer::RendererBackend::Direct3D11;
        }
        if (svName == "vulkan")
        {
            return Flawy::Renderer::RendererBackend::Vulkan;
        }
        throw std::invalid_argument("Unknown renderer '" + std::string(svName) + "', expected d3d11 or vulkan");
    }

    Flawy::Renderer::RendererBackend ParseRendererBackend(std::span<char*> spanArguments)
    {
        Flawy::Renderer::RendererBackend eBackend = Flawy::Renderer::RendererBackend::Default;
        if (spanArguments.empty())
        {
            return eBackend;
        }

        for (const std::string_view svArgument : spanArguments.subspan(1))
        {
            if (svArgument.starts_with(kRendererOption))
            {
                eBackend = ParseRendererName(svArgument.substr(kRendererOption.size()));
            }
        }
        return eBackend;
    }

    int ReportFatalError(const std::exception& oException)
    {
        Flawy::Core::Logger::Initialize("Flawy-Sandbox");
        Flawy::Core::Logger::Get().critical("{}", oException.what());
        Flawy::Core::Logger::Shutdown();
        return 1;
    }
}

int main(int iArgumentCount, char* aszArguments[])
{
    try
    {
        const std::span<char*> spanArguments(aszArguments, static_cast<std::size_t>(iArgumentCount));
        Sandbox::SandboxApplication oApplication(ParseRendererBackend(spanArguments));
        return oApplication.Run();
    }
    catch (const std::exception& oException)
    {
        return ReportFatalError(oException);
    }
}
