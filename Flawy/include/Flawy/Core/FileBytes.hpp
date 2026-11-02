#pragma once

#include <cstddef>
#include <filesystem>
#include <span>
#include <vector>

namespace Flawy::Core
{
    class FileBytes final
    {
    public:
        FileBytes() = delete;

        [[nodiscard]] static std::vector<std::byte> Read(const std::filesystem::path& oPath);
        static void WriteAtomically(const std::filesystem::path& oPath, std::span<const std::byte> spanBytes);
    };
}
