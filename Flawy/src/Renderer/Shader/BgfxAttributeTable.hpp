#pragma once

#include "Flawy/Renderer/VertexLayout.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace Flawy::Renderer
{
    class BgfxAttributeTable final
    {
    public:
        static constexpr uint16_t kUnknownAttributeId = 0xffff;

        BgfxAttributeTable() = delete;

        [[nodiscard]] static std::optional<VertexAttribute> FromSemantic(std::string_view svSemanticName, size_t uSemanticIndex);
        [[nodiscard]] static uint16_t ToAttributeId(VertexAttribute eAttribute) noexcept;
        [[nodiscard]] static std::optional<VertexAttribute> FromAttributeId(uint16_t uAttributeId) noexcept;
        [[nodiscard]] static bool IsPredefinedUniform(std::string_view svName) noexcept;
    };
}
