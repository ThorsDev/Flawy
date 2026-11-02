#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <unordered_set>

namespace Flawy::Import
{
    class AssetNameRegistry final
    {
    public:
        static constexpr size_t kMaxNameLength = 96;

        [[nodiscard]] std::string Reserve(std::string_view svName, std::string_view svFallback);
        [[nodiscard]] static std::string Sanitize(std::string_view svName);

    private:
        [[nodiscard]] static std::string ToLower(std::string_view svName);

        std::unordered_set<std::string> m_setLowerNames;
    };
}
