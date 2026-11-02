#pragma once

#include "Flawy/Core/Logger.hpp"

#include <format>
#include <utility>

namespace Flawy::Import
{
    class ImportLog final
    {
    public:
        ImportLog() = delete;

        template <typename... TArguments>
        static void Info(std::format_string<TArguments...> oFormat, TArguments&&... oArguments)
        {
            if (Core::Logger::IsInitialized())
            {
                Core::Logger::Get().info("{}", std::format(oFormat, std::forward<TArguments>(oArguments)...));
            }
        }

        template <typename... TArguments>
        static void Warn(std::format_string<TArguments...> oFormat, TArguments&&... oArguments)
        {
            if (Core::Logger::IsInitialized())
            {
                Core::Logger::Get().warn("{}", std::format(oFormat, std::forward<TArguments>(oArguments)...));
            }
        }

        template <typename... TArguments>
        static void Debug(std::format_string<TArguments...> oFormat, TArguments&&... oArguments)
        {
            if (Core::Logger::IsInitialized())
            {
                Core::Logger::Get().debug("{}", std::format(oFormat, std::forward<TArguments>(oArguments)...));
            }
        }
    };
}
