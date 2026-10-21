#include "Flawy/Renderer/Material/MaterialGraph.hpp"

#include "Renderer/Material/MaterialNodeTraits.hpp"
#include "Renderer/Material/MaterialText.hpp"

#include <format>
#include <string>
#include <stdexcept>
#include <utility>

namespace Flawy::Renderer
{
    namespace
    {
        void RequireUtf8(std::string_view svText, std::string_view svWhat)
        {
            if (!MaterialText::IsValidUtf8(svText))
            {
                throw std::invalid_argument(std::format("{} must be valid UTF-8", svWhat));
            }
        }

        void RequireName(std::string_view svName, MaterialNodeKind eKind)
        {
            if (svName.empty())
            {
                throw std::invalid_argument(std::format("{} nodes need a non-empty name", MaterialNodeTraits::GetName(eKind)));
            }
            RequireUtf8(svName, std::format("{} name", MaterialNodeTraits::GetName(eKind)));
        }
    }

    bool MaterialNodeId::IsValid() const noexcept
    {
        return uIndex != kInvalid;
    }

    MaterialNodeId MaterialGraph::AddConstant(const glm::vec4& vecValue, MaterialValueType eType)
    {
        MaterialNode oNode;
        oNode.eKind = MaterialNodeKind::Constant;
        oNode.eType = eType;
        oNode.vecValue = vecValue;
        return AddNode(std::move(oNode));
    }

    MaterialNodeId MaterialGraph::AddScalarParameter(std::string_view svName, float fDefault)
    {
        RequireName(svName, MaterialNodeKind::ScalarParameter);
        MaterialNode oNode;
        oNode.eKind = MaterialNodeKind::ScalarParameter;
        oNode.sName = std::string(svName);
        oNode.vecValue = glm::vec4(fDefault, 0.0f, 0.0f, 0.0f);
        return AddNode(std::move(oNode));
    }

    MaterialNodeId MaterialGraph::AddVectorParameter(std::string_view svName, const glm::vec4& vecDefault, MaterialValueType eType)
    {
        RequireName(svName, MaterialNodeKind::VectorParameter);
        MaterialNode oNode;
        oNode.eKind = MaterialNodeKind::VectorParameter;
        oNode.sName = std::string(svName);
        oNode.eType = eType;
        oNode.vecValue = vecDefault;
        return AddNode(std::move(oNode));
    }

    MaterialNodeId MaterialGraph::AddTextureParameter(std::string_view svName, bool bSrgb)
    {
        RequireName(svName, MaterialNodeKind::TextureParameter);
        MaterialNode oNode;
        oNode.eKind = MaterialNodeKind::TextureParameter;
        oNode.sName = std::string(svName);
        oNode.bSrgb = bSrgb;
        return AddNode(std::move(oNode));
    }

    MaterialNodeId MaterialGraph::AddTextureSample(MaterialNodeId oTexture, MaterialNodeId oTexCoord)
    {
        RequireNode(oTexture);
        RequireOptionalNode(oTexCoord);
        if (GetNode(oTexture).eKind != MaterialNodeKind::TextureParameter)
        {
            throw std::invalid_argument(std::format("TextureSample input {} must be a TextureParameter node", oTexture.uIndex));
        }

        MaterialNode oNode;
        oNode.eKind = MaterialNodeKind::TextureSample;
        oNode.aInputs = {oTexture, oTexCoord, MaterialNodeId{}};
        return AddNode(std::move(oNode));
    }

    MaterialNodeId MaterialGraph::AddTexCoord(uint8_t uIndex)
    {
        if (uIndex != 0)
        {
            throw std::invalid_argument(std::format("TexCoord index {} is not supported; only TEXCOORD0 is available", uIndex));
        }

        MaterialNode oNode;
        oNode.eKind = MaterialNodeKind::TexCoord;
        oNode.uIndex = uIndex;
        return AddNode(std::move(oNode));
    }

    MaterialNodeId MaterialGraph::AddVertexColor()
    {
        MaterialNode oNode;
        oNode.eKind = MaterialNodeKind::VertexColor;
        return AddNode(std::move(oNode));
    }

    MaterialNodeId MaterialGraph::AddVertexNormal()
    {
        MaterialNode oNode;
        oNode.eKind = MaterialNodeKind::VertexNormal;
        return AddNode(std::move(oNode));
    }

    MaterialNodeId MaterialGraph::AddTime()
    {
        MaterialNode oNode;
        oNode.eKind = MaterialNodeKind::Time;
        return AddNode(std::move(oNode));
    }

    MaterialNodeId MaterialGraph::AddUnary(MaterialNodeKind eKind, MaterialNodeId oInput)
    {
        if (!MaterialNodeTraits::IsUnary(eKind))
        {
            throw std::invalid_argument(std::format("{} is not a unary node kind", MaterialNodeTraits::GetName(eKind)));
        }
        RequireNode(oInput);

        MaterialNode oNode;
        oNode.eKind = eKind;
        oNode.aInputs[0] = oInput;
        return AddNode(std::move(oNode));
    }

    MaterialNodeId MaterialGraph::AddBinary(MaterialNodeKind eKind, MaterialNodeId oLeft, MaterialNodeId oRight)
    {
        if (!MaterialNodeTraits::IsBinary(eKind))
        {
            throw std::invalid_argument(std::format("{} is not a binary node kind", MaterialNodeTraits::GetName(eKind)));
        }
        RequireNode(oLeft);
        RequireNode(oRight);

        MaterialNode oNode;
        oNode.eKind = eKind;
        oNode.aInputs = {oLeft, oRight, MaterialNodeId{}};
        return AddNode(std::move(oNode));
    }

    MaterialNodeId MaterialGraph::AddLerp(MaterialNodeId oFrom, MaterialNodeId oTo, MaterialNodeId oAlpha)
    {
        RequireNode(oFrom);
        RequireNode(oTo);
        RequireNode(oAlpha);

        MaterialNode oNode;
        oNode.eKind = MaterialNodeKind::Lerp;
        oNode.aInputs = {oFrom, oTo, oAlpha};
        return AddNode(std::move(oNode));
    }

    MaterialNodeId MaterialGraph::AddComponentMask(MaterialNodeId oInput, bool bR, bool bG, bool bB, bool bA)
    {
        RequireNode(oInput);
        if (!bR && !bG && !bB && !bA)
        {
            throw std::invalid_argument("ComponentMask must keep at least one component");
        }

        MaterialNode oNode;
        oNode.eKind = MaterialNodeKind::ComponentMask;
        oNode.aInputs[0] = oInput;
        oNode.aMask = {bR, bG, bB, bA};
        return AddNode(std::move(oNode));
    }

    MaterialNodeId MaterialGraph::AddAppend(MaterialNodeId oLeft, MaterialNodeId oRight)
    {
        RequireNode(oLeft);
        RequireNode(oRight);

        MaterialNode oNode;
        oNode.eKind = MaterialNodeKind::Append;
        oNode.aInputs = {oLeft, oRight, MaterialNodeId{}};
        return AddNode(std::move(oNode));
    }

    MaterialNodeId MaterialGraph::AddFresnel(MaterialNodeId oExponent, float fDefaultExponent)
    {
        RequireOptionalNode(oExponent);

        MaterialNode oNode;
        oNode.eKind = MaterialNodeKind::Fresnel;
        oNode.aInputs[0] = oExponent;
        oNode.vecValue = glm::vec4(fDefaultExponent, 0.0f, 0.0f, 0.0f);
        return AddNode(std::move(oNode));
    }

    MaterialNodeId MaterialGraph::AddStaticSwitch(std::string_view svName, MaterialNodeId oWhenTrue, MaterialNodeId oWhenFalse, bool bDefault)
    {
        RequireName(svName, MaterialNodeKind::StaticSwitch);
        RequireNode(oWhenTrue);
        RequireNode(oWhenFalse);

        MaterialNode oNode;
        oNode.eKind = MaterialNodeKind::StaticSwitch;
        oNode.sName = std::string(svName);
        oNode.aInputs = {oWhenTrue, oWhenFalse, MaterialNodeId{}};
        oNode.vecValue = MaterialNodeTraits::MakeStaticSwitchValue(bDefault);
        return AddNode(std::move(oNode));
    }

    MaterialNodeId MaterialGraph::AddCustomCode(std::string_view svExpression, std::span<const MaterialNodeId> spanInputs, MaterialValueType eOutputType)
    {
        if (spanInputs.size() > MaterialNode::kMaxInputs)
        {
            throw std::invalid_argument(std::format("CustomCode accepts at most {} inputs", MaterialNode::kMaxInputs));
        }

        RequireUtf8(svExpression, "CustomCode expression");

        MaterialNode oNode;
        oNode.eKind = MaterialNodeKind::CustomCode;
        oNode.eType = eOutputType;
        oNode.sCode = std::string(svExpression);
        for (size_t uIndex = 0; uIndex < spanInputs.size(); ++uIndex)
        {
            RequireNode(spanInputs[uIndex]);
            oNode.aInputs[uIndex] = spanInputs[uIndex];
        }
        return AddNode(std::move(oNode));
    }

    void MaterialGraph::ConnectOutput(MaterialOutputSlot eSlot, MaterialNodeId oNode)
    {
        RequireNode(oNode);
        m_aOutputs[static_cast<size_t>(eSlot)] = oNode;
    }

    void MaterialGraph::DisconnectOutput(MaterialOutputSlot eSlot) noexcept
    {
        m_aOutputs[static_cast<size_t>(eSlot)] = MaterialNodeId{};
    }

    void MaterialGraph::SetShadingModel(MaterialShadingModel eModel) noexcept
    {
        m_eShadingModel = eModel;
    }

    void MaterialGraph::SetName(std::string_view svName)
    {
        RequireUtf8(svName, "Material graph name");
        m_sName = std::string(svName);
    }

    std::span<const MaterialNode> MaterialGraph::GetNodes() const noexcept
    {
        return m_vNodes;
    }

    const MaterialNode& MaterialGraph::GetNode(MaterialNodeId oId) const
    {
        RequireNode(oId);
        return m_vNodes[oId.uIndex];
    }

    MaterialNodeId MaterialGraph::GetOutput(MaterialOutputSlot eSlot) const noexcept
    {
        return m_aOutputs[static_cast<size_t>(eSlot)];
    }

    MaterialShadingModel MaterialGraph::GetShadingModel() const noexcept
    {
        return m_eShadingModel;
    }

    std::string_view MaterialGraph::GetName() const noexcept
    {
        return m_sName;
    }

    MaterialNodeId MaterialGraph::AddNode(MaterialNode oNode)
    {
        if (m_vNodes.size() >= MaterialNodeId::kInvalid)
        {
            throw std::length_error("Material graph has too many nodes");
        }
        m_vNodes.push_back(std::move(oNode));
        return MaterialNodeId{static_cast<uint32_t>(m_vNodes.size() - 1)};
    }

    void MaterialGraph::RequireNode(MaterialNodeId oId) const
    {
        if (!oId.IsValid() || oId.uIndex >= m_vNodes.size())
        {
            throw std::out_of_range(std::format("Material graph has no node {}", oId.IsValid() ? std::to_string(oId.uIndex) : std::string("<invalid>")));
        }
    }

    void MaterialGraph::RequireOptionalNode(MaterialNodeId oId) const
    {
        if (oId.IsValid())
        {
            RequireNode(oId);
        }
    }
}
