#include "Renderer/Bgfx/BgfxVertexLayoutBuilder.hpp"

#include <stdexcept>

namespace Flawy::Renderer
{
    bgfx::VertexLayout BgfxVertexLayoutBuilder::Build(const VertexLayout& oLayout) const
    {
        if (oLayout.GetElements().empty())
        {
            throw std::invalid_argument("VertexLayout has no elements");
        }

        bgfx::VertexLayout oBgfxLayout;
        oBgfxLayout.begin();
        for (const VertexLayoutElement& oElement : oLayout.GetElements())
        {
            oBgfxLayout.add(ToBgfxAttribute(oElement.eAttribute), oElement.uCount, ToBgfxAttributeType(oElement.eType), oElement.bNormalized);
        }
        oBgfxLayout.end();
        return oBgfxLayout;
    }

    bgfx::Attrib::Enum BgfxVertexLayoutBuilder::ToBgfxAttribute(VertexAttribute eAttribute) noexcept
    {
        switch (eAttribute)
        {
        case VertexAttribute::Position:
            return bgfx::Attrib::Position;
        case VertexAttribute::Normal:
            return bgfx::Attrib::Normal;
        case VertexAttribute::Tangent:
            return bgfx::Attrib::Tangent;
        case VertexAttribute::Bitangent:
            return bgfx::Attrib::Bitangent;
        case VertexAttribute::Color0:
            return bgfx::Attrib::Color0;
        case VertexAttribute::Color1:
            return bgfx::Attrib::Color1;
        case VertexAttribute::Color2:
            return bgfx::Attrib::Color2;
        case VertexAttribute::Color3:
            return bgfx::Attrib::Color3;
        case VertexAttribute::Indices:
            return bgfx::Attrib::Indices;
        case VertexAttribute::Weight:
            return bgfx::Attrib::Weight;
        case VertexAttribute::TexCoord0:
            return bgfx::Attrib::TexCoord0;
        case VertexAttribute::TexCoord1:
            return bgfx::Attrib::TexCoord1;
        case VertexAttribute::TexCoord2:
            return bgfx::Attrib::TexCoord2;
        case VertexAttribute::TexCoord3:
            return bgfx::Attrib::TexCoord3;
        case VertexAttribute::TexCoord4:
            return bgfx::Attrib::TexCoord4;
        case VertexAttribute::TexCoord5:
            return bgfx::Attrib::TexCoord5;
        case VertexAttribute::TexCoord6:
            return bgfx::Attrib::TexCoord6;
        case VertexAttribute::TexCoord7:
            return bgfx::Attrib::TexCoord7;
        }
        return bgfx::Attrib::Count;
    }

    bgfx::AttribType::Enum BgfxVertexLayoutBuilder::ToBgfxAttributeType(VertexAttributeType eType) noexcept
    {
        switch (eType)
        {
        case VertexAttributeType::Uint8:
            return bgfx::AttribType::Uint8;
        case VertexAttributeType::Int16:
            return bgfx::AttribType::Int16;
        case VertexAttributeType::Half:
            return bgfx::AttribType::Half;
        case VertexAttributeType::Float:
            return bgfx::AttribType::Float;
        }
        return bgfx::AttribType::Count;
    }
}
