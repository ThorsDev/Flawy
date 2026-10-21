#include "Renderer/Material/MaterialGraphValidator.hpp"

#include "Renderer/Material/MaterialCustomCodeRules.hpp"
#include "Renderer/Material/MaterialGraphTraversal.hpp"

#include <cmath>
#include <format>
#include <optional>
#include <string>
#include <unordered_map>

namespace Flawy::Renderer
{
    namespace
    {
        bool IsFinite(const glm::vec4& vecValue) noexcept
        {
            return std::isfinite(vecValue.x) && std::isfinite(vecValue.y) && std::isfinite(vecValue.z) && std::isfinite(vecValue.w);
        }

        void ValidateCustomCode(const MaterialWorkingGraph& oGraph, uint32_t uNode)
        {
            const MaterialNode& oNode = oGraph.vNodes[uNode];
            const std::optional<std::string> optProblem = MaterialCustomCodeRules::FindProblem(oNode.sCode, MaterialNodeTraits::GetUsedInputCount(oNode));
            if (optProblem)
            {
                oGraph.Fail(uNode, *optProblem);
            }
        }
    }

    void MaterialGraphValidator::Validate(const MaterialWorkingGraph& oGraph) const
    {
        if (oGraph.vNodes.size() > MaterialWorkingGraph::kMaxNodeCount)
        {
            throw MaterialCompileError(MaterialNodeId{}, std::format("Material graph has {} nodes; at most {} are supported", oGraph.vNodes.size(), MaterialWorkingGraph::kMaxNodeCount));
        }

        const auto uNodeCount = static_cast<uint32_t>(oGraph.vNodes.size());
        std::unordered_map<std::string, uint32_t> mapParameterNames;
        std::unordered_map<std::string, uint32_t> mapSwitchNames;

        for (uint32_t uNode = 0; uNode < uNodeCount; ++uNode)
        {
            const MaterialNode& oNode = oGraph.vNodes[uNode];
            const MaterialNodeSignature oSignature = MaterialNodeTraits::GetSignature(oNode.eKind);
            const uint8_t uUsedInputs = MaterialNodeTraits::GetUsedInputCount(oNode);
            for (uint8_t uInput = 0; uInput < uUsedInputs; ++uInput)
            {
                const MaterialNodeId oInput = oNode.aInputs[uInput];
                if (!oInput.IsValid())
                {
                    if (uInput < oSignature.uRequiredInputs)
                    {
                        oGraph.Fail(uNode, std::format("input {} is not connected", uInput));
                    }
                    continue;
                }
                if (oInput.uIndex >= uNodeCount)
                {
                    oGraph.Fail(uNode, std::format("input {} references missing node {}", uInput, oInput.uIndex));
                }

                const bool bTextureInput = oGraph.vNodes[oInput.uIndex].eKind == MaterialNodeKind::TextureParameter;
                const bool bTextureSlot = oNode.eKind == MaterialNodeKind::TextureSample && uInput == 0;
                if (bTextureInput != bTextureSlot)
                {
                    oGraph.Fail(uNode, bTextureSlot ? "input 0 must be a TextureParameter node" : std::format("input {} is a TextureParameter, which can only feed a TextureSample", uInput));
                }
            }

            if (!IsFinite(oNode.vecValue))
            {
                oGraph.Fail(uNode, "value must be finite");
            }
            if (MaterialNodeTraits::IsParameter(oNode.eKind))
            {
                if (oNode.sName.empty())
                {
                    oGraph.Fail(uNode, "parameter name must not be empty");
                }
                const auto [itExisting, bInserted] = mapParameterNames.emplace(oNode.sName, uNode);
                if (!bInserted)
                {
                    oGraph.Fail(uNode, std::format("parameter name '{}' is already used by node {}", oNode.sName, itExisting->second));
                }
            }

            if (oNode.eKind == MaterialNodeKind::StaticSwitch)
            {
                if (oNode.sName.empty())
                {
                    oGraph.Fail(uNode, "static switch name must not be empty");
                }
                const auto [itExisting, bInserted] = mapSwitchNames.emplace(oNode.sName, uNode);
                if (!bInserted)
                {
                    oGraph.Fail(uNode, std::format("static switch name '{}' is already used by node {}", oNode.sName, itExisting->second));
                }
            }

            switch (oNode.eKind)
            {
            case MaterialNodeKind::TexCoord:
                if (oNode.uIndex != 0)
                {
                    oGraph.Fail(uNode, "only TEXCOORD0 is supported");
                }
                break;
            case MaterialNodeKind::ComponentMask:
                if (!oNode.aMask[0] && !oNode.aMask[1] && !oNode.aMask[2] && !oNode.aMask[3])
                {
                    oGraph.Fail(uNode, "must keep at least one component");
                }
                break;
            case MaterialNodeKind::CustomCode:
                ValidateCustomCode(oGraph, uNode);
                break;
            default:
                break;
            }
        }

        for (size_t uSlot = 0; uSlot < MaterialGraph::kOutputSlotCount; ++uSlot)
        {
            const MaterialNodeId oOutput = oGraph.aOutputs[uSlot];
            if (oOutput.IsValid() && oOutput.uIndex >= uNodeCount)
            {
                throw MaterialCompileError(oOutput, std::format("Material graph output {} references missing node {}", MaterialNodeTraits::GetSlotName(static_cast<MaterialOutputSlot>(uSlot)), oOutput.uIndex));
            }
        }

        std::vector<uint32_t> vAllNodes(uNodeCount);
        for (uint32_t uNode = 0; uNode < uNodeCount; ++uNode)
        {
            vAllNodes[uNode] = uNode;
        }
        const MaterialTraversalResult oTraversal = MaterialGraphTraversal::PostOrder(oGraph.vNodes, vAllNodes);
        if (oTraversal.optCycleNode)
        {
            oGraph.Fail(*oTraversal.optCycleNode, "is part of a cycle");
        }
    }
}
