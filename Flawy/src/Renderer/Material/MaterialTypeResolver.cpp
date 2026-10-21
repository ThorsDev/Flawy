#include "Renderer/Material/MaterialTypeResolver.hpp"

#include <format>

namespace Flawy::Renderer
{
    namespace
    {
        std::string DescribeComponents(uint8_t uComponents)
        {
            return uComponents == MaterialWorkingGraph::kTextureComponents ? std::string("Texture") : std::format("Float{}", uComponents);
        }

        class TypeResolution final
        {
        public:
            explicit TypeResolution(MaterialWorkingGraph& oGraph) noexcept
                : m_oGraph(oGraph)
            {
            }

            void Resolve(uint32_t uNode)
            {
                m_oGraph.vComponents[uNode] = Compute(uNode);
            }

        private:
            [[nodiscard]] uint8_t Input(uint32_t uNode, uint8_t uInput) const
            {
                return m_oGraph.vComponents[m_oGraph.vNodes[uNode].aInputs[uInput].uIndex];
            }

            uint8_t Broadcast(uint32_t uNode, uint8_t uLeft, uint8_t uRight) const
            {
                if (uLeft == uRight || uRight == 1)
                {
                    return uLeft;
                }
                if (uLeft == 1)
                {
                    return uRight;
                }
                m_oGraph.Fail(uNode, std::format("cannot combine {} and {}", DescribeComponents(uLeft), DescribeComponents(uRight)));
            }

            void RequireScalarOr(uint32_t uNode, uint8_t uActual, uint8_t uAllowed, std::string_view svRole) const
            {
                if (uActual != 1 && uActual != uAllowed)
                {
                    m_oGraph.Fail(uNode, std::format("{} must be Float1 or {} but is {}", svRole, DescribeComponents(uAllowed), DescribeComponents(uActual)));
                }
            }

            uint8_t Compute(uint32_t uNode)
            {
                const MaterialNode& oNode = m_oGraph.vNodes[uNode];
                switch (oNode.eKind)
                {
                case MaterialNodeKind::Constant:
                case MaterialNodeKind::VectorParameter:
                case MaterialNodeKind::CustomCode:
                    return MaterialNodeTraits::GetComponentCount(oNode.eType);
                case MaterialNodeKind::ScalarParameter:
                case MaterialNodeKind::Time:
                    return 1;
                case MaterialNodeKind::TextureParameter:
                    return MaterialWorkingGraph::kTextureComponents;
                case MaterialNodeKind::TextureSample:
                    if (oNode.aInputs[1].IsValid() && Input(uNode, 1) != 2)
                    {
                        m_oGraph.Fail(uNode, std::format("texture coordinate must be Float2 but is {}", DescribeComponents(Input(uNode, 1))));
                    }
                    return 4;
                case MaterialNodeKind::TexCoord:
                    return 2;
                case MaterialNodeKind::VertexColor:
                    return 4;
                case MaterialNodeKind::VertexNormal:
                    return 3;
                case MaterialNodeKind::Add:
                case MaterialNodeKind::Subtract:
                case MaterialNodeKind::Multiply:
                case MaterialNodeKind::Divide:
                    return Broadcast(uNode, Input(uNode, 0), Input(uNode, 1));
                case MaterialNodeKind::Dot:
                    static_cast<void>(Broadcast(uNode, Input(uNode, 0), Input(uNode, 1)));
                    return 1;
                case MaterialNodeKind::Lerp:
                {
                    const uint8_t uResult = Broadcast(uNode, Input(uNode, 0), Input(uNode, 1));
                    RequireScalarOr(uNode, Input(uNode, 2), uResult, "alpha");
                    return uResult;
                }
                case MaterialNodeKind::Power:
                {
                    const uint8_t uBase = Input(uNode, 0);
                    RequireScalarOr(uNode, Input(uNode, 1), uBase, "exponent");
                    return uBase;
                }
                case MaterialNodeKind::Normalize:
                    if (Input(uNode, 0) < 2)
                    {
                        m_oGraph.Fail(uNode, "requires a vector input (Float2 or wider)");
                    }
                    return Input(uNode, 0);
                case MaterialNodeKind::Saturate:
                case MaterialNodeKind::OneMinus:
                case MaterialNodeKind::Abs:
                case MaterialNodeKind::Sin:
                case MaterialNodeKind::Cos:
                    return Input(uNode, 0);
                case MaterialNodeKind::ComponentMask:
                    return ResolveMask(uNode, Input(uNode, 0));
                case MaterialNodeKind::Append:
                {
                    const uint8_t uSum = static_cast<uint8_t>(Input(uNode, 0) + Input(uNode, 1));
                    if (uSum > 4)
                    {
                        m_oGraph.Fail(uNode, std::format("appending {} and {} exceeds Float4", DescribeComponents(Input(uNode, 0)), DescribeComponents(Input(uNode, 1))));
                    }
                    return uSum;
                }
                case MaterialNodeKind::StaticSwitch:
                    return ResolveSwitch(uNode, Input(uNode, 0), Input(uNode, 1));
                case MaterialNodeKind::Fresnel:
                    if (oNode.aInputs[0].IsValid() && Input(uNode, 0) != 1)
                    {
                        m_oGraph.Fail(uNode, std::format("exponent must be Float1 but is {}", DescribeComponents(Input(uNode, 0))));
                    }
                    return 1;
                }
                return 1;
            }

            uint8_t ResolveSwitch(uint32_t uNode, uint8_t uWhenTrue, uint8_t uWhenFalse) const
            {
                const bool bTexture = uWhenTrue == MaterialWorkingGraph::kTextureComponents || uWhenFalse == MaterialWorkingGraph::kTextureComponents;
                const bool bCompatible = uWhenTrue == uWhenFalse || uWhenTrue == 1 || uWhenFalse == 1;
                if (bTexture || !bCompatible)
                {
                    m_oGraph.Fail(uNode,
                                  std::format("static switch '{}' branches must have the same type (or Float1) but are {} and {}",
                                              m_oGraph.vNodes[uNode].sName,
                                              DescribeComponents(uWhenTrue),
                                              DescribeComponents(uWhenFalse)));
                }
                return uWhenTrue == 1 ? uWhenFalse : uWhenTrue;
            }

            uint8_t ResolveMask(uint32_t uNode, uint8_t uInputComponents) const
            {
                const MaterialNode& oNode = m_oGraph.vNodes[uNode];
                uint8_t uCount = 0;
                for (uint8_t uChannel = 0; uChannel < 4; ++uChannel)
                {
                    if (!oNode.aMask[uChannel])
                    {
                        continue;
                    }
                    if (uChannel >= uInputComponents)
                    {
                        m_oGraph.Fail(uNode, std::format("selects component {} of a {} input", "rgba"[uChannel], DescribeComponents(uInputComponents)));
                    }
                    ++uCount;
                }
                return uCount;
            }

            MaterialWorkingGraph& m_oGraph;
        };
    }

    void MaterialTypeResolver::Resolve(MaterialWorkingGraph& oGraph) const
    {
        ResolveOrder(oGraph, oGraph.vOrder);
    }

    void MaterialTypeResolver::ResolveLayout(MaterialWorkingGraph& oGraph) const
    {
        ResolveOrder(oGraph, oGraph.vLayoutOrder);
    }

    void MaterialTypeResolver::ResolveOrder(MaterialWorkingGraph& oGraph, std::span<const uint32_t> spanOrder)
    {
        TypeResolution oResolution(oGraph);
        for (const uint32_t uNode : spanOrder)
        {
            oResolution.Resolve(uNode);
        }

        for (size_t uSlot = 0; uSlot < MaterialGraph::kOutputSlotCount; ++uSlot)
        {
            const MaterialNodeId oOutput = oGraph.aOutputs[uSlot];
            if (!oOutput.IsValid())
            {
                continue;
            }
            const auto eSlot = static_cast<MaterialOutputSlot>(uSlot);
            const uint8_t uWanted = MaterialNodeTraits::GetSlotComponentCount(eSlot);
            const uint8_t uActual = oGraph.vComponents[oOutput.uIndex];
            if (uActual == MaterialWorkingGraph::kTextureComponents || (uActual != 1 && uActual < uWanted))
            {
                oGraph.Fail(oOutput.uIndex, std::format("output {} needs Float{}{} but receives {}", MaterialNodeTraits::GetSlotName(eSlot), uWanted, uWanted > 1 ? " (or Float1)" : "", DescribeComponents(uActual)));
            }
        }
    }
}
