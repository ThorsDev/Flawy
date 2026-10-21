#include "Renderer/Material/MaterialConstantFolder.hpp"

#include <glm/common.hpp>
#include <glm/exponential.hpp>
#include <glm/trigonometric.hpp>

#include <algorithm>
#include <cmath>
#include <optional>

namespace Flawy::Renderer
{
    namespace
    {
        bool IsFoldable(MaterialNodeKind eKind) noexcept
        {
            switch (eKind)
            {
            case MaterialNodeKind::Add:
            case MaterialNodeKind::Subtract:
            case MaterialNodeKind::Multiply:
            case MaterialNodeKind::Divide:
            case MaterialNodeKind::Lerp:
            case MaterialNodeKind::Dot:
            case MaterialNodeKind::Saturate:
            case MaterialNodeKind::OneMinus:
            case MaterialNodeKind::Power:
            case MaterialNodeKind::Normalize:
            case MaterialNodeKind::Abs:
            case MaterialNodeKind::Sin:
            case MaterialNodeKind::Cos:
            case MaterialNodeKind::ComponentMask:
            case MaterialNodeKind::Append:
                return true;
            default:
                return false;
            }
        }

        glm::vec4 Truncate(const glm::vec4& vecValue, uint8_t uComponents) noexcept
        {
            glm::vec4 vecResult(0.0f);
            for (uint8_t uComponent = 0; uComponent < uComponents; ++uComponent)
            {
                vecResult[uComponent] = vecValue[uComponent];
            }
            return vecResult;
        }

        glm::vec4 Expand(const glm::vec4& vecValue, uint8_t uFrom, uint8_t uTo) noexcept
        {
            return Truncate(uFrom == 1 ? glm::vec4(vecValue.x) : vecValue, uTo);
        }

        bool IsFinite(const glm::vec4& vecValue) noexcept
        {
            return std::isfinite(vecValue.x) && std::isfinite(vecValue.y) && std::isfinite(vecValue.z) && std::isfinite(vecValue.w);
        }

        class ConstantEvaluator final
        {
        public:
            explicit ConstantEvaluator(MaterialWorkingGraph& oGraph) noexcept
                : m_oGraph(oGraph)
            {
            }

            void Fold(uint32_t uNode)
            {
                MaterialNode& oNode = m_oGraph.vNodes[uNode];
                const uint8_t uCount = MaterialNodeTraits::GetUsedInputCount(oNode);
                bool bAllConstant = uCount > 0;
                for (uint8_t uInput = 0; uInput < uCount; ++uInput)
                {
                    const MaterialNodeId oInput = oNode.aInputs[uInput];
                    bAllConstant = bAllConstant && oInput.IsValid() && m_oGraph.vNodes[oInput.uIndex].eKind == MaterialNodeKind::Constant;
                }
                if (!bAllConstant || !IsFoldable(oNode.eKind))
                {
                    return;
                }

                const std::optional<glm::vec4> optValue = Evaluate(uNode);
                if (!optValue || !IsFinite(*optValue))
                {
                    return;
                }

                const uint8_t uComponents = m_oGraph.vComponents[uNode];
                MaterialNode oConstant;
                oConstant.eKind = MaterialNodeKind::Constant;
                oConstant.eType = MaterialNodeTraits::FromComponentCount(uComponents);
                oConstant.vecValue = Truncate(*optValue, uComponents);
                oNode = oConstant;
            }

        private:
            [[nodiscard]] glm::vec4 Operand(uint32_t uNode, uint8_t uInput, uint8_t uWidth) const
            {
                const uint32_t uSource = m_oGraph.vNodes[uNode].aInputs[uInput].uIndex;
                return Expand(m_oGraph.vNodes[uSource].vecValue, m_oGraph.vComponents[uSource], uWidth);
            }

            [[nodiscard]] uint8_t Width(uint32_t uNode, uint8_t uInput) const
            {
                return m_oGraph.vComponents[m_oGraph.vNodes[uNode].aInputs[uInput].uIndex];
            }

            [[nodiscard]] std::optional<glm::vec4> Evaluate(uint32_t uNode) const
            {
                const MaterialNode& oNode = m_oGraph.vNodes[uNode];
                const uint8_t uWidth = m_oGraph.vComponents[uNode];
                switch (oNode.eKind)
                {
                case MaterialNodeKind::Add:
                    return Operand(uNode, 0, uWidth) + Operand(uNode, 1, uWidth);
                case MaterialNodeKind::Subtract:
                    return Operand(uNode, 0, uWidth) - Operand(uNode, 1, uWidth);
                case MaterialNodeKind::Multiply:
                    return Operand(uNode, 0, uWidth) * Operand(uNode, 1, uWidth);
                case MaterialNodeKind::Divide:
                    return Truncate(Operand(uNode, 0, uWidth) / Expand(Operand(uNode, 1, uWidth), uWidth, 4), uWidth);
                case MaterialNodeKind::Lerp:
                {
                    const glm::vec4 vecFrom = Operand(uNode, 0, uWidth);
                    return vecFrom + (Operand(uNode, 1, uWidth) - vecFrom) * Operand(uNode, 2, uWidth);
                }
                case MaterialNodeKind::Dot:
                {
                    const uint8_t uOperandWidth = std::max(Width(uNode, 0), Width(uNode, 1));
                    const glm::vec4 vecProduct = Operand(uNode, 0, uOperandWidth) * Operand(uNode, 1, uOperandWidth);
                    return glm::vec4(vecProduct.x + vecProduct.y + vecProduct.z + vecProduct.w, 0.0f, 0.0f, 0.0f);
                }
                case MaterialNodeKind::Saturate:
                    return Truncate(glm::clamp(Operand(uNode, 0, uWidth), 0.0f, 1.0f), uWidth);
                case MaterialNodeKind::OneMinus:
                    return Truncate(glm::vec4(1.0f) - Operand(uNode, 0, uWidth), uWidth);
                case MaterialNodeKind::Power:
                {
                    const glm::vec4 vecBase = Operand(uNode, 0, uWidth);
                    const glm::vec4 vecExponent = Operand(uNode, 1, uWidth);
                    for (uint8_t uComponent = 0; uComponent < uWidth; ++uComponent)
                    {
                        const bool bNegativeBase = vecBase[uComponent] < 0.0f;
                        const bool bZeroBaseNonPositiveExponent = vecBase[uComponent] == 0.0f && vecExponent[uComponent] <= 0.0f;
                        if (bNegativeBase || bZeroBaseNonPositiveExponent)
                        {
                            return std::nullopt;
                        }
                    }
                    return Truncate(glm::pow(vecBase, vecExponent), uWidth);
                }
                case MaterialNodeKind::Normalize:
                {
                    const glm::vec4 vecValue = Operand(uNode, 0, uWidth);
                    return vecValue / std::sqrt(vecValue.x * vecValue.x + vecValue.y * vecValue.y + vecValue.z * vecValue.z + vecValue.w * vecValue.w);
                }
                case MaterialNodeKind::Abs:
                    return glm::abs(Operand(uNode, 0, uWidth));
                case MaterialNodeKind::Sin:
                    return Truncate(glm::sin(Operand(uNode, 0, uWidth)), uWidth);
                case MaterialNodeKind::Cos:
                    return Truncate(glm::cos(Operand(uNode, 0, uWidth)), uWidth);
                case MaterialNodeKind::ComponentMask:
                {
                    const glm::vec4 vecInput = Operand(uNode, 0, Width(uNode, 0));
                    glm::vec4 vecResult(0.0f);
                    int iTarget = 0;
                    for (int iChannel = 0; iChannel < 4; ++iChannel)
                    {
                        if (oNode.aMask[static_cast<size_t>(iChannel)])
                        {
                            vecResult[iTarget++] = vecInput[iChannel];
                        }
                    }
                    return vecResult;
                }
                case MaterialNodeKind::Append:
                {
                    const uint8_t uLeftWidth = Width(uNode, 0);
                    const glm::vec4 vecLeft = Operand(uNode, 0, uLeftWidth);
                    const glm::vec4 vecRight = Operand(uNode, 1, Width(uNode, 1));
                    glm::vec4 vecResult = vecLeft;
                    for (uint8_t uComponent = uLeftWidth; uComponent < uWidth; ++uComponent)
                    {
                        vecResult[uComponent] = vecRight[uComponent - uLeftWidth];
                    }
                    return vecResult;
                }
                default:
                    return std::nullopt;
                }
            }

            MaterialWorkingGraph& m_oGraph;
        };
    }

    void MaterialConstantFolder::Fold(MaterialWorkingGraph& oGraph) const
    {
        ConstantEvaluator oEvaluator(oGraph);
        for (const uint32_t uNode : oGraph.vOrder)
        {
            oEvaluator.Fold(uNode);
        }
    }
}
