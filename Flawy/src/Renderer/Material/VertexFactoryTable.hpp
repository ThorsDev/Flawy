#pragma once

#include "Flawy/Renderer/Material/MaterialPass.hpp"
#include "Flawy/Renderer/VertexLayout.hpp"

#include <string_view>

namespace Flawy::Renderer
{
    class VertexFactoryTable final
    {
    public:
        VertexFactoryTable() = delete;

        [[nodiscard]] static std::string_view GetName(VertexFactoryKind eKind) noexcept;
        [[nodiscard]] static std::string_view GetTypeName(VertexFactoryKind eKind) noexcept;
        [[nodiscard]] static VertexFactoryKind FromLayout(const VertexLayout& oLayout);
    };
}
