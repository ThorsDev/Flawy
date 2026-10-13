#pragma once

#include <spdlog/logger.h>

#include <memory>
#include <string_view>

namespace Flawy::Core
{
    class Logger final
    {
    public:
        Logger() = delete;

        static void Initialize(std::string_view svName);
        static void Shutdown() noexcept;

        [[nodiscard]] static bool IsInitialized() noexcept;
        [[nodiscard]] static spdlog::logger& Get();

    private:
        static std::shared_ptr<spdlog::logger> s_spLogger;
    };
}
