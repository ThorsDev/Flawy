#pragma once

#include <cstddef>
#include <functional>
#include <string_view>

namespace Flawy::Core
{
    struct TransparentStringHash
    {
        using is_transparent = void;

        [[nodiscard]] size_t operator()(std::string_view svText) const noexcept
        {
            return std::hash<std::string_view>{}(svText);
        }
    };
}
