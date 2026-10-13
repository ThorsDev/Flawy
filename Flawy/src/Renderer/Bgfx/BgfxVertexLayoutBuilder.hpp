#pragma once

#include "Flawy/Renderer/VertexLayout.hpp"

#include <bgfx/bgfx.h>

namespace Flawy::Renderer
{
    class BgfxVertexLayoutBuilder final
    {
    public:
        [[nodiscard]] bgfx::VertexLayout Build(const VertexLayout& oLayout) const;

        [[nodiscard]] static bgfx::Attrib::Enum ToBgfxAttribute(VertexAttribute eAttribute) noexcept;
        [[nodiscard]] static bgfx::AttribType::Enum ToBgfxAttributeType(VertexAttributeType eType) noexcept;
    };
}
