#pragma once

#include <cstdint>

namespace Flawy::Renderer
{
    struct RenderGraphResource
    {
        static constexpr uint16_t kInvalidIndex = 0xffffu;
        static constexpr uint32_t kPersistentGeneration = 0;

        uint16_t uIndex = kInvalidIndex;
        uint32_t uGeneration = kPersistentGeneration;

        [[nodiscard]] bool IsValid() const noexcept
        {
            return uIndex != kInvalidIndex;
        }

        [[nodiscard]] bool operator==(const RenderGraphResource&) const noexcept = default;
    };
}
