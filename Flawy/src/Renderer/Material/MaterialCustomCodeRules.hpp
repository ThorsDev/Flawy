#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace Flawy::Renderer
{
    class MaterialCustomCodeRules final
    {
    public:
        MaterialCustomCodeRules() = delete;

        [[nodiscard]] static std::optional<std::string> FindProblem(std::string_view svCode, uint8_t uInputCount);
        [[nodiscard]] static bool UsesIdentifier(std::string_view svCode, std::string_view svIdentifier) noexcept;
    };
}
