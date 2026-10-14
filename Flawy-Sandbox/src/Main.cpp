#include "SandboxApplication.hpp"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

namespace
{
    constexpr std::string_view kRendererOption = "--renderer=";
    constexpr std::string_view kFramesOption = "--frames=";
    constexpr std::string_view kScreenshotOption = "--screenshot=";

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

    uint32_t ParseFrameCount(std::string_view svValue)
    {
        uint32_t uFrames = 0;
        const char* pEnd = svValue.data() + svValue.size();
        const std::from_chars_result oResult = std::from_chars(svValue.data(), pEnd, uFrames);
        if (svValue.empty() || oResult.ec != std::errc{} || oResult.ptr != pEnd)
        {
            throw std::invalid_argument("Invalid frame count '" + std::string(svValue) + "', expected a non-negative integer");
        }
        return uFrames;
    }

    Sandbox::SandboxOptions ParseOptions(std::span<char*> spanArguments)
    {
        Sandbox::SandboxOptions oOptions;
        if (spanArguments.empty())
        {
            return oOptions;
        }

        for (const std::string_view svArgument : spanArguments.subspan(1))
        {
            if (svArgument.starts_with(kRendererOption))
            {
                oOptions.eBackend = ParseRendererName(svArgument.substr(kRendererOption.size()));
            }
            else if (svArgument.starts_with(kFramesOption))
            {
                oOptions.uFrameLimit = ParseFrameCount(svArgument.substr(kFramesOption.size()));
            }
            else if (svArgument.starts_with(kScreenshotOption))
            {
                const std::string_view svPath = svArgument.substr(kScreenshotOption.size());
                if (svPath.empty())
                {
                    throw std::invalid_argument("--screenshot requires a file path");
                }
                oOptions.optScreenshotPath = std::filesystem::path(svPath);
            }
        }
        return oOptions;
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
        Sandbox::SandboxApplication oApplication(ParseOptions(spanArguments));
        return oApplication.Run();
    }
    catch (const std::exception& oException)
    {
        return ReportFatalError(oException);
    }
}
