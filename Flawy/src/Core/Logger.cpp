#include "Flawy/Core/Logger.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <stdexcept>
#include <string>

namespace Flawy::Core
{
    std::shared_ptr<spdlog::logger> Logger::s_spLogger;

    void Logger::Initialize(std::string_view svName)
    {
        if (s_spLogger)
        {
            return;
        }

        s_spLogger = spdlog::stdout_color_mt(std::string(svName));
        s_spLogger->set_pattern("%^[%T] [%n] [%l]%$ %v");
        s_spLogger->set_level(spdlog::level::trace);
    }

    void Logger::Shutdown() noexcept
    {
        if (!s_spLogger)
        {
            return;
        }

        s_spLogger->flush();
        spdlog::drop(s_spLogger->name());
        s_spLogger.reset();
    }

    bool Logger::IsInitialized() noexcept
    {
        return s_spLogger != nullptr;
    }

    spdlog::logger& Logger::Get()
    {
        if (!s_spLogger)
        {
            throw std::logic_error("Logger::Get called before Logger::Initialize");
        }

        return *s_spLogger;
    }
}
