#include "Renderer/Material/MaterialSlangEmitter.hpp"

#include "Renderer/Material/MaterialShaderNames.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <optional>
#include <span>
#include <vector>

namespace Flawy::Renderer
{
    namespace
    {
        constexpr std::string_view kSwizzleChannels = "xyzw";
        constexpr std::string_view kBodyIndent = "        ";

        std::string FormatFloat(float fValue)
        {
            std::string sText = std::format("{}", fValue);
            if (sText.find_first_of(".eE") == std::string::npos)
            {
                sText.append(".0");
            }
            return sText;
        }

        std::string TypeName(uint8_t uComponents)
        {
            return uComponents == 1 ? std::string("float") : std::format("float{}", uComponents);
        }

        std::string Literal(const glm::vec4& vecValue, uint8_t uComponents)
        {
            if (uComponents == 1)
            {
                return FormatFloat(vecValue.x);
            }

            std::string sText = TypeName(uComponents) + "(";
            for (uint8_t uComponent = 0; uComponent < uComponents; ++uComponent)
            {
                sText.append(uComponent == 0 ? "" : ", ");
                sText.append(FormatFloat(vecValue[uComponent]));
            }
            sText.push_back(')');
            return sText;
        }

        std::string Broadcast(const std::string& sExpression, uint8_t uFrom, uint8_t uTo)
        {
            if (uFrom == uTo)
            {
                return sExpression;
            }
            return std::format("(({})({}))", TypeName(uTo), sExpression);
        }

        std::string Fit(const std::string& sExpression, uint8_t uFrom, uint8_t uTo)
        {
            if (uFrom == uTo || uFrom == 1)
            {
                return Broadcast(sExpression, uFrom, uTo);
            }
            return std::format("({}).{}", sExpression, kSwizzleChannels.substr(0, uTo));
        }

        std::string_view GetBinaryOperator(MaterialNodeKind eKind) noexcept
        {
            switch (eKind)
            {
            case MaterialNodeKind::Add:
                return "+";
            case MaterialNodeKind::Subtract:
                return "-";
            case MaterialNodeKind::Multiply:
                return "*";
            default:
                return "/";
            }
        }

        std::string_view GetUnaryFunction(MaterialNodeKind eKind) noexcept
        {
            switch (eKind)
            {
            case MaterialNodeKind::Saturate:
                return "saturate";
            case MaterialNodeKind::Normalize:
                return "normalize";
            case MaterialNodeKind::Abs:
                return "abs";
            case MaterialNodeKind::Sin:
                return "sin";
            default:
                return "cos";
            }
        }

        class FunctionEmitter final
        {
        public:
            FunctionEmitter(const MaterialWorkingGraph& oGraph, const MaterialPackedParameters& oParameters, bool bVertexStage)
                : m_oGraph(oGraph)
                , m_oParameters(oParameters)
                , m_bVertexStage(bVertexStage)
                , m_vExpressions(oGraph.vNodes.size())
            {
            }

            [[nodiscard]] std::string Expression(uint32_t uNode)
            {
                if (m_vExpressions[uNode])
                {
                    return *m_vExpressions[uNode];
                }

                const std::optional<std::string> optInline = InlineExpression(uNode);
                if (optInline)
                {
                    m_vExpressions[uNode] = optInline;
                    return *optInline;
                }

                const std::string sValue = ComputedExpression(uNode);
                const uint8_t uComponents = m_oGraph.vComponents[uNode];
                const std::string sVariable = std::format("{}Node{}", uComponents == 1 ? "f" : "vec", uNode);
                m_sBody.append(std::format("{}{} {} = {};\n", kBodyIndent, TypeName(uComponents), sVariable, sValue));
                m_vExpressions[uNode] = sVariable;
                return sVariable;
            }

            void EmitDependencies(std::span<const uint32_t> spanRoots)
            {
                std::vector<bool> vNeeded(m_oGraph.vNodes.size(), false);
                for (const uint32_t uRoot : spanRoots)
                {
                    vNeeded[uRoot] = true;
                }
                for (auto itNode = m_oGraph.vOrder.rbegin(); itNode != m_oGraph.vOrder.rend(); ++itNode)
                {
                    if (!vNeeded[*itNode])
                    {
                        continue;
                    }
                    const MaterialNode& oNode = m_oGraph.vNodes[*itNode];
                    const uint8_t uCount = MaterialNodeTraits::GetUsedInputCount(oNode);
                    for (uint8_t uInput = 0; uInput < uCount; ++uInput)
                    {
                        if (oNode.aInputs[uInput].IsValid())
                        {
                            vNeeded[oNode.aInputs[uInput].uIndex] = true;
                        }
                    }
                }
                for (const uint32_t uNode : m_oGraph.vOrder)
                {
                    if (vNeeded[uNode])
                    {
                        static_cast<void>(Expression(uNode));
                    }
                }
            }

            void AppendStatement(std::string_view svStatement)
            {
                m_sBody.append(kBodyIndent);
                m_sBody.append(svStatement);
                m_sBody.push_back('\n');
            }

            [[nodiscard]] const std::string& GetBody() const noexcept
            {
                return m_sBody;
            }

        private:
            [[nodiscard]] uint8_t Width(uint32_t uNode, uint8_t uInput) const
            {
                return m_oGraph.vComponents[m_oGraph.vNodes[uNode].aInputs[uInput].uIndex];
            }

            [[nodiscard]] std::string Input(uint32_t uNode, uint8_t uInput)
            {
                return Expression(m_oGraph.vNodes[uNode].aInputs[uInput].uIndex);
            }

            [[nodiscard]] std::string InputAs(uint32_t uNode, uint8_t uInput, uint8_t uWidth)
            {
                return Broadcast(Input(uNode, uInput), Width(uNode, uInput), uWidth);
            }

            [[nodiscard]] std::optional<std::string> InlineExpression(uint32_t uNode) const
            {
                const MaterialNode& oNode = m_oGraph.vNodes[uNode];
                const uint8_t uComponents = m_oGraph.vComponents[uNode];
                switch (oNode.eKind)
                {
                case MaterialNodeKind::Constant:
                    return Literal(oNode.vecValue, uComponents);
                case MaterialNodeKind::ScalarParameter:
                case MaterialNodeKind::VectorParameter:
                    return std::format("{}[{}].{}", MaterialShaderNames::kMaterialParamsUniform, m_oParameters.vSlots[uNode], kSwizzleChannels.substr(m_oParameters.vFirstComponents[uNode], uComponents));
                case MaterialNodeKind::TextureParameter:
                    return MaterialShaderNames::GetTextureSamplerName(m_oParameters.vTextureStages[uNode]);
                case MaterialNodeKind::TexCoord:
                    return std::string("oInput.vecTexCoord0");
                case MaterialNodeKind::VertexColor:
                    return std::string("oInput.vecVertexColor");
                case MaterialNodeKind::VertexNormal:
                    return std::string(m_bVertexStage ? "vecWorldNormal" : "oInput.vecWorldNormal");
                case MaterialNodeKind::Time:
                    return std::string("oInput.fTimeSeconds");
                default:
                    return std::nullopt;
                }
            }

            [[nodiscard]] std::string ComputedExpression(uint32_t uNode)
            {
                const MaterialNode& oNode = m_oGraph.vNodes[uNode];
                const uint8_t uWidth = m_oGraph.vComponents[uNode];
                switch (oNode.eKind)
                {
                case MaterialNodeKind::TextureSample:
                {
                    const std::string sCoordinate = oNode.aInputs[1].IsValid() ? Input(uNode, 1) : std::string("oInput.vecTexCoord0");
                    return std::format("FLAWY_SAMPLE2D({}, {})", Input(uNode, 0), sCoordinate);
                }
                case MaterialNodeKind::Add:
                case MaterialNodeKind::Subtract:
                case MaterialNodeKind::Multiply:
                case MaterialNodeKind::Divide:
                    return std::format("({} {} {})", InputAs(uNode, 0, uWidth), GetBinaryOperator(oNode.eKind), InputAs(uNode, 1, uWidth));
                case MaterialNodeKind::Lerp:
                    return std::format("lerp({}, {}, {})", InputAs(uNode, 0, uWidth), InputAs(uNode, 1, uWidth), InputAs(uNode, 2, uWidth));
                case MaterialNodeKind::Dot:
                {
                    const uint8_t uOperandWidth = std::max(Width(uNode, 0), Width(uNode, 1));
                    if (uOperandWidth == 1)
                    {
                        return std::format("({} * {})", Input(uNode, 0), Input(uNode, 1));
                    }
                    return std::format("dot({}, {})", InputAs(uNode, 0, uOperandWidth), InputAs(uNode, 1, uOperandWidth));
                }
                case MaterialNodeKind::OneMinus:
                    return std::format("(1.0 - {})", Input(uNode, 0));
                case MaterialNodeKind::Saturate:
                case MaterialNodeKind::Normalize:
                case MaterialNodeKind::Abs:
                case MaterialNodeKind::Sin:
                case MaterialNodeKind::Cos:
                    return std::format("{}({})", GetUnaryFunction(oNode.eKind), Input(uNode, 0));
                case MaterialNodeKind::Power:
                    return std::format("pow({}, {})", Input(uNode, 0), InputAs(uNode, 1, uWidth));
                case MaterialNodeKind::ComponentMask:
                    return MaskExpression(uNode);
                case MaterialNodeKind::Append:
                    return std::format("{}({}, {})", TypeName(uWidth), Input(uNode, 0), Input(uNode, 1));
                case MaterialNodeKind::Fresnel:
                {
                    const std::string sExponent = oNode.aInputs[0].IsValid() ? Input(uNode, 0) : FormatFloat(oNode.vecValue.x);
                    return std::format("pow(1.0 - saturate(dot(oInput.vecWorldNormal, oInput.vecViewDirection)), {})", sExponent);
                }
                case MaterialNodeKind::CustomCode:
                    return std::format("(({})({}))", TypeName(uWidth), SubstituteCustomCode(uNode));
                default:
                    m_oGraph.Fail(uNode, "cannot be emitted");
                }
            }

            [[nodiscard]] std::string MaskExpression(uint32_t uNode)
            {
                const MaterialNode& oNode = m_oGraph.vNodes[uNode];
                const std::string sInput = Input(uNode, 0);
                if (Width(uNode, 0) == 1)
                {
                    return sInput;
                }

                std::string sSwizzle;
                for (size_t uChannel = 0; uChannel < oNode.aMask.size(); ++uChannel)
                {
                    if (oNode.aMask[uChannel])
                    {
                        sSwizzle.push_back(kSwizzleChannels[uChannel]);
                    }
                }
                return std::format("({}).{}", sInput, sSwizzle);
            }

            [[nodiscard]] std::string SubstituteCustomCode(uint32_t uNode)
            {
                const std::string& sCode = m_oGraph.vNodes[uNode].sCode;
                std::string sResult;
                for (size_t uPosition = 0; uPosition < sCode.size(); ++uPosition)
                {
                    if (sCode[uPosition] == '$' && uPosition + 1 < sCode.size())
                    {
                        const auto uInput = static_cast<uint8_t>(sCode[uPosition + 1] - '0');
                        sResult.append("(" + Input(uNode, uInput) + ")");
                        ++uPosition;
                        continue;
                    }
                    sResult.push_back(sCode[uPosition]);
                }
                return sResult;
            }

            const MaterialWorkingGraph& m_oGraph;
            const MaterialPackedParameters& m_oParameters;
            bool m_bVertexStage;
            std::vector<std::optional<std::string>> m_vExpressions;
            std::string m_sBody;
        };

        struct SurfaceField
        {
            MaterialOutputSlot eSlot;
            std::string_view svField;
        };

        constexpr std::array<SurfaceField, 7> kSurfaceFields = {{
            {MaterialOutputSlot::BaseColor, "vecBaseColor"},
            {MaterialOutputSlot::Normal, "vecNormal"},
            {MaterialOutputSlot::Metallic, "fMetallic"},
            {MaterialOutputSlot::Roughness, "fRoughness"},
            {MaterialOutputSlot::AmbientOcclusion, "fAmbientOcclusion"},
            {MaterialOutputSlot::Emissive, "vecEmissive"},
            {MaterialOutputSlot::Opacity, "fOpacity"},
        }};

        std::string EmitSurfaceFunction(const MaterialWorkingGraph& oGraph, const MaterialPackedParameters& oParameters)
        {
            FunctionEmitter oEmitter(oGraph, oParameters, false);
            oEmitter.AppendStatement("SurfaceOutput oOutput = MakeDefaultSurface();");
            std::vector<uint32_t> vRoots;
            for (const SurfaceField& oField : kSurfaceFields)
            {
                const MaterialNodeId oOutput = oGraph.GetOutput(oField.eSlot);
                if (oOutput.IsValid())
                {
                    vRoots.push_back(oOutput.uIndex);
                }
            }
            oEmitter.EmitDependencies(vRoots);
            for (const SurfaceField& oField : kSurfaceFields)
            {
                const MaterialNodeId oOutput = oGraph.GetOutput(oField.eSlot);
                if (!oOutput.IsValid())
                {
                    continue;
                }
                const std::string sValue = Fit(oEmitter.Expression(oOutput.uIndex), oGraph.vComponents[oOutput.uIndex], MaterialNodeTraits::GetSlotComponentCount(oField.eSlot));
                const std::string sAssigned = oField.eSlot == MaterialOutputSlot::Normal ? std::format("normalize({})", sValue) : sValue;
                oEmitter.AppendStatement(std::format("oOutput.{} = {};", oField.svField, sAssigned));
            }
            oEmitter.AppendStatement("return oOutput;");
            return oEmitter.GetBody();
        }

        std::string EmitWorldPositionOffsetFunction(const MaterialWorkingGraph& oGraph, const MaterialPackedParameters& oParameters)
        {
            FunctionEmitter oEmitter(oGraph, oParameters, true);
            const MaterialNodeId oOutput = oGraph.GetOutput(MaterialOutputSlot::WorldPositionOffset);
            if (!oOutput.IsValid())
            {
                oEmitter.AppendStatement("return float3(0.0, 0.0, 0.0);");
                return oEmitter.GetBody();
            }
            const std::array<uint32_t, 1> aRoot = {oOutput.uIndex};
            oEmitter.EmitDependencies(aRoot);
            const std::string sValue = Fit(oEmitter.Expression(oOutput.uIndex), oGraph.vComponents[oOutput.uIndex], 3);
            oEmitter.AppendStatement(std::format("return {};", sValue));
            return oEmitter.GetBody();
        }
    }

    std::string MaterialSlangEmitter::Emit(const MaterialWorkingGraph& oGraph, const MaterialPackedParameters& oParameters, std::string_view svModuleName) const
    {
        std::string sSource = std::format("module {};\n\nimport Flawy.MaterialInterface;\n#include \"Flawy/Sampler.slang\"\n\n", svModuleName);
        if (oParameters.oLayout.uVec4SlotCount > 0)
        {
            sSource.append(std::format("uniform float4 {}[{}];\n\n", MaterialShaderNames::kMaterialParamsUniform, oParameters.oLayout.uVec4SlotCount));
        }
        for (const MaterialTextureParameter& oTexture : oParameters.oLayout.vTextures)
        {
            sSource.append(std::format("FLAWY_SAMPLER2D({}, {});\n", MaterialShaderNames::GetTextureSamplerName(oTexture.uStage), oTexture.uStage));
        }
        if (!oParameters.oLayout.vTextures.empty())
        {
            sSource.push_back('\n');
        }

        const bool bTangentSpaceNormal = oGraph.GetOutput(MaterialOutputSlot::Normal).IsValid();
        sSource.append(std::format("public struct {} : IMaterial\n{{\n", svModuleName));
        sSource.append(std::format("    static const ShadingModel kShadingModel = ShadingModel::{};\n", MaterialNodeTraits::GetShadingModelName(oGraph.eShadingModel)));
        sSource.append(std::format("    static const bool kUsesTangentSpaceNormal = {};\n\n", bTangentSpaceNormal ? "true" : "false"));
        sSource.append("    static SurfaceOutput EvaluateSurface(SurfaceInput oInput)\n    {\n");
        sSource.append(EmitSurfaceFunction(oGraph, oParameters));
        sSource.append("    }\n\n    static float3 EvaluateWorldPositionOffset(float3 vecWorldPosition, float3 vecWorldNormal)\n    {\n");
        sSource.append(EmitWorldPositionOffsetFunction(oGraph, oParameters));
        sSource.append("    }\n}\n");
        return sSource;
    }
}
