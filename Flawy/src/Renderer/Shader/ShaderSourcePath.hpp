#pragma once

#include <string>
#include <string_view>

namespace Flawy::Renderer
{
    class ShaderSourcePath final
    {
    public:
        ShaderSourcePath() = delete;

        [[nodiscard]] static std::string Normalize(std::string_view svPath);
        [[nodiscard]] static std::string_view GetModuleName(std::string_view svPath) noexcept;
        [[nodiscard]] static bool IsContainedRelativePath(std::string_view svPath) noexcept;
    };
}
