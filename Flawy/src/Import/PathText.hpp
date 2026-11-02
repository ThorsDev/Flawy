#pragma once

#include <filesystem>
#include <string>

namespace Flawy::Import
{
    class PathText final
    {
    public:
        PathText() = delete;

        [[nodiscard]] static std::string ToUtf8(const std::filesystem::path& oPath);
        [[nodiscard]] static std::string GetStem(const std::filesystem::path& oPath);
    };
}
