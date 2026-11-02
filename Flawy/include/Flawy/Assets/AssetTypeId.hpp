#pragma once

#include "Flawy/Core/Hash.hpp"

#include <cstdint>
#include <string_view>

namespace Flawy::Assets
{
    struct AssetTypeId
    {
        uint64_t uValue = 0;

        [[nodiscard]] static constexpr AssetTypeId FromName(std::string_view svName) noexcept
        {
            return {Core::Fnv1aHasher::HashText(svName)};
        }

        [[nodiscard]] constexpr bool operator==(const AssetTypeId&) const noexcept = default;
    };
}
