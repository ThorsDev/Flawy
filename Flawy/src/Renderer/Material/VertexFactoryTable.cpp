#include "Renderer/Material/VertexFactoryTable.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>

namespace Flawy::Renderer
{
    namespace
    {
        struct RequiredAttribute
        {
            VertexAttribute eAttribute = VertexAttribute::Position;
            uint8_t uCount = 0;
        };

        constexpr std::array<RequiredAttribute, 4> kStaticMeshAttributes = {{
            {VertexAttribute::Position, 3},
            {VertexAttribute::Normal, 3},
            {VertexAttribute::TexCoord0, 2},
            {VertexAttribute::Color0, 4},
        }};

        constexpr RequiredAttribute kTangentAttribute = {VertexAttribute::Tangent, 4};

        std::string_view ToAttributeName(VertexAttribute eAttribute) noexcept
        {
            constexpr std::array<std::string_view, 18> kNames = {
                "Position", "Normal", "Tangent", "Bitangent", "Color0", "Color1", "Color2", "Color3", "Indices",
                "Weight", "TexCoord0", "TexCoord1", "TexCoord2", "TexCoord3", "TexCoord4", "TexCoord5", "TexCoord6", "TexCoord7",
            };
            return kNames[static_cast<size_t>(eAttribute)];
        }

        std::optional<VertexLayoutElement> FindElement(const VertexLayout& oLayout, VertexAttribute eAttribute) noexcept
        {
            const std::span<const VertexLayoutElement> spanElements = oLayout.GetElements();
            const auto itElement = std::ranges::find(spanElements, eAttribute, &VertexLayoutElement::eAttribute);
            if (itElement == spanElements.end())
            {
                return std::nullopt;
            }
            return *itElement;
        }

        class LayoutProblemList final
        {
        public:
            void Check(const VertexLayout& oLayout, const RequiredAttribute& oRequired)
            {
                const std::optional<VertexLayoutElement> optElement = FindElement(oLayout, oRequired.eAttribute);
                if (!optElement)
                {
                    m_sMissing.append(m_sMissing.empty() ? "" : ", ");
                    m_sMissing.append(ToAttributeName(oRequired.eAttribute));
                    return;
                }
                if (optElement->uCount != oRequired.uCount)
                {
                    m_sMismatched.append(m_sMismatched.empty() ? "" : ", ");
                    m_sMismatched.append(std::format("{} has {} component(s) but needs {}", ToAttributeName(oRequired.eAttribute), optElement->uCount, oRequired.uCount));
                }
            }

            void ThrowIfAny(VertexFactoryKind eKind) const
            {
                if (m_sMissing.empty() && m_sMismatched.empty())
                {
                    return;
                }

                std::string sMessage = std::format("Vertex layout cannot feed the {} vertex factory;", VertexFactoryTable::GetName(eKind));
                if (!m_sMissing.empty())
                {
                    sMessage.append(std::format(" missing attribute(s): {};", m_sMissing));
                }
                if (!m_sMismatched.empty())
                {
                    sMessage.append(std::format(" wrong component count: {};", m_sMismatched));
                }
                sMessage.pop_back();
                throw std::logic_error(sMessage);
            }

        private:
            std::string m_sMissing;
            std::string m_sMismatched;
        };
    }

    std::string_view VertexFactoryTable::GetName(VertexFactoryKind eKind) noexcept
    {
        switch (eKind)
        {
        case VertexFactoryKind::StaticMesh:
            return "StaticMesh";
        case VertexFactoryKind::StaticMeshTangent:
            return "StaticMeshTangent";
        }
        return "Unknown";
    }

    std::string_view VertexFactoryTable::GetTypeName(VertexFactoryKind eKind) noexcept
    {
        switch (eKind)
        {
        case VertexFactoryKind::StaticMesh:
            return "StaticMeshVertexFactory";
        case VertexFactoryKind::StaticMeshTangent:
            return "StaticMeshTangentVertexFactory";
        }
        return "Unknown";
    }

    VertexFactoryKind VertexFactoryTable::FromLayout(const VertexLayout& oLayout)
    {
        const bool bHasTangent = FindElement(oLayout, kTangentAttribute.eAttribute).has_value();
        const VertexFactoryKind eKind = bHasTangent ? VertexFactoryKind::StaticMeshTangent : VertexFactoryKind::StaticMesh;

        LayoutProblemList oProblems;
        for (const RequiredAttribute& oRequired : kStaticMeshAttributes)
        {
            oProblems.Check(oLayout, oRequired);
        }
        if (bHasTangent)
        {
            oProblems.Check(oLayout, kTangentAttribute);
        }
        oProblems.ThrowIfAny(eKind);
        return eKind;
    }
}
