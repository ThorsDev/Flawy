#include "Renderer/Shader/BgfxAttributeTable.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <string>

namespace Flawy::Renderer
{
    namespace
    {
        struct AttributeEntry
        {
            VertexAttribute eAttribute;
            std::string_view svSemantic;
            size_t uSemanticIndex;
            uint16_t uId;
        };

        constexpr std::array kAttributes = {
            AttributeEntry{VertexAttribute::Position, "POSITION", 0, 0x0001},
            AttributeEntry{VertexAttribute::Normal, "NORMAL", 0, 0x0002},
            AttributeEntry{VertexAttribute::Tangent, "TANGENT", 0, 0x0003},
            AttributeEntry{VertexAttribute::Bitangent, "BITANGENT", 0, 0x0004},
            AttributeEntry{VertexAttribute::Color0, "COLOR", 0, 0x0005},
            AttributeEntry{VertexAttribute::Color1, "COLOR", 1, 0x0006},
            AttributeEntry{VertexAttribute::Color2, "COLOR", 2, 0x0018},
            AttributeEntry{VertexAttribute::Color3, "COLOR", 3, 0x0019},
            AttributeEntry{VertexAttribute::Indices, "BLENDINDICES", 0, 0x000e},
            AttributeEntry{VertexAttribute::Weight, "BLENDWEIGHT", 0, 0x000f},
            AttributeEntry{VertexAttribute::TexCoord0, "TEXCOORD", 0, 0x0010},
            AttributeEntry{VertexAttribute::TexCoord1, "TEXCOORD", 1, 0x0011},
            AttributeEntry{VertexAttribute::TexCoord2, "TEXCOORD", 2, 0x0012},
            AttributeEntry{VertexAttribute::TexCoord3, "TEXCOORD", 3, 0x0013},
            AttributeEntry{VertexAttribute::TexCoord4, "TEXCOORD", 4, 0x0014},
            AttributeEntry{VertexAttribute::TexCoord5, "TEXCOORD", 5, 0x0015},
            AttributeEntry{VertexAttribute::TexCoord6, "TEXCOORD", 6, 0x0016},
            AttributeEntry{VertexAttribute::TexCoord7, "TEXCOORD", 7, 0x0017},
        };

        constexpr std::array<std::string_view, 14> kPredefinedUniforms = {
            "u_viewRect",
            "u_viewTexel",
            "u_view",
            "u_invView",
            "u_proj",
            "u_invProj",
            "u_viewProj",
            "u_invViewProj",
            "u_model",
            "u_modelView",
            "u_invModelView",
            "u_modelViewProj",
            "u_alphaRef4",
            "bgfx_indirectArgBase",
        };

        struct NormalizedSemantic
        {
            std::string sName;
            size_t uIndex = 0;
        };

        NormalizedSemantic NormalizeSemantic(std::string_view svSemanticName, size_t uSemanticIndex)
        {
            NormalizedSemantic oSemantic;
            oSemantic.uIndex = uSemanticIndex;
            for (const char cCharacter : svSemanticName)
            {
                oSemantic.sName.push_back(static_cast<char>(std::toupper(static_cast<unsigned char>(cCharacter))));
            }

            const size_t uDigitStart = oSemantic.sName.find_last_not_of("0123456789") + 1;
            if (uDigitStart < oSemantic.sName.size() && uSemanticIndex == 0)
            {
                size_t uParsedIndex = 0;
                const char* szBegin = oSemantic.sName.data() + uDigitStart;
                const char* szEnd = oSemantic.sName.data() + oSemantic.sName.size();
                if (std::from_chars(szBegin, szEnd, uParsedIndex).ec == std::errc{})
                {
                    oSemantic.uIndex = uParsedIndex;
                    oSemantic.sName.resize(uDigitStart);
                }
            }
            return oSemantic;
        }

        template <typename TPredicate>
        const AttributeEntry* FindAttribute(TPredicate fnPredicate) noexcept
        {
            for (const AttributeEntry& oEntry : kAttributes)
            {
                if (fnPredicate(oEntry))
                {
                    return &oEntry;
                }
            }
            return nullptr;
        }
    }

    std::optional<VertexAttribute> BgfxAttributeTable::FromSemantic(std::string_view svSemanticName, size_t uSemanticIndex)
    {
        const NormalizedSemantic oSemantic = NormalizeSemantic(svSemanticName, uSemanticIndex);
        const AttributeEntry* pEntry = FindAttribute([&oSemantic](const AttributeEntry& oEntry) {
            return oEntry.svSemantic == oSemantic.sName && oEntry.uSemanticIndex == oSemantic.uIndex;
        });
        if (pEntry == nullptr)
        {
            return std::nullopt;
        }
        return pEntry->eAttribute;
    }

    uint16_t BgfxAttributeTable::ToAttributeId(VertexAttribute eAttribute) noexcept
    {
        const AttributeEntry* pEntry = FindAttribute([eAttribute](const AttributeEntry& oEntry) { return oEntry.eAttribute == eAttribute; });
        return pEntry == nullptr ? kUnknownAttributeId : pEntry->uId;
    }

    std::optional<VertexAttribute> BgfxAttributeTable::FromAttributeId(uint16_t uAttributeId) noexcept
    {
        const AttributeEntry* pEntry = FindAttribute([uAttributeId](const AttributeEntry& oEntry) { return oEntry.uId == uAttributeId; });
        if (pEntry == nullptr)
        {
            return std::nullopt;
        }
        return pEntry->eAttribute;
    }

    bool BgfxAttributeTable::IsPredefinedUniform(std::string_view svName) noexcept
    {
        return std::ranges::find(kPredefinedUniforms, svName) != kPredefinedUniforms.end();
    }
}
