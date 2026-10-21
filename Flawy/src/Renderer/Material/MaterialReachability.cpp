#include "Renderer/Material/MaterialReachability.hpp"

#include "Renderer/Material/MaterialCustomCodeRules.hpp"
#include "Renderer/Material/MaterialGraphTraversal.hpp"

#include <algorithm>
#include <array>
#include <format>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

namespace Flawy::Renderer
{
    namespace
    {
        constexpr std::string_view kSurfaceInputIdentifier = "oInput";

        bool IsAvailableInVertexStage(MaterialNodeKind eKind) noexcept
        {
            switch (eKind)
            {
            case MaterialNodeKind::TextureParameter:
            case MaterialNodeKind::TextureSample:
            case MaterialNodeKind::TexCoord:
            case MaterialNodeKind::VertexColor:
            case MaterialNodeKind::Time:
            case MaterialNodeKind::Fresnel:
                return false;
            default:
                return true;
            }
        }

        std::vector<uint32_t> Traverse(const MaterialWorkingGraph& oGraph, std::span<const uint32_t> spanRoots)
        {
            MaterialTraversalResult oTraversal = MaterialGraphTraversal::PostOrder(oGraph.vNodes, spanRoots);
            if (oTraversal.optCycleNode)
            {
                oGraph.Fail(*oTraversal.optCycleNode, "is part of a cycle");
            }
            return std::move(oTraversal.vPostOrder);
        }

        std::vector<uint32_t> CollectOutputRoots(const MaterialWorkingGraph& oGraph)
        {
            std::vector<uint32_t> vRoots;
            for (const MaterialNodeId& oOutput : oGraph.aOutputs)
            {
                if (oOutput.IsValid())
                {
                    vRoots.push_back(oOutput.uIndex);
                }
            }
            return vRoots;
        }

        void CheckVertexStageNode(const MaterialWorkingGraph& oGraph, uint32_t uNode)
        {
            const MaterialNode& oNode = oGraph.vNodes[uNode];
            if (!IsAvailableInVertexStage(oNode.eKind))
            {
                oGraph.Fail(uNode, std::format("{} cannot feed WorldPositionOffset", MaterialNodeTraits::GetName(oNode.eKind)));
            }
            if (oNode.eKind == MaterialNodeKind::CustomCode && MaterialCustomCodeRules::UsesIdentifier(oNode.sCode, kSurfaceInputIdentifier))
            {
                oGraph.Fail(uNode, std::format("CustomCode feeding WorldPositionOffset cannot use '{}'; only vecWorldPosition and vecWorldNormal are in scope", kSurfaceInputIdentifier));
            }
        }
    }

    void MaterialReachability::Mark(MaterialWorkingGraph& oGraph) const
    {
        std::fill(oGraph.vReachable.begin(), oGraph.vReachable.end(), false);
        std::fill(oGraph.vVertexStage.begin(), oGraph.vVertexStage.end(), false);

        oGraph.vOrder = Traverse(oGraph, CollectOutputRoots(oGraph));
        for (const uint32_t uNode : oGraph.vOrder)
        {
            oGraph.vReachable[uNode] = true;
        }

        const MaterialNodeId oOffset = oGraph.GetOutput(MaterialOutputSlot::WorldPositionOffset);
        if (!oOffset.IsValid())
        {
            return;
        }
        const std::array<uint32_t, 1> aOffsetRoot = {oOffset.uIndex};
        for (const uint32_t uNode : Traverse(oGraph, aOffsetRoot))
        {
            oGraph.vVertexStage[uNode] = true;
            CheckVertexStageNode(oGraph, uNode);
        }
    }

    void MaterialReachability::MarkLayout(MaterialWorkingGraph& oGraph) const
    {
        std::fill(oGraph.vLayoutReachable.begin(), oGraph.vLayoutReachable.end(), false);
        oGraph.vLayoutOrder = Traverse(oGraph, CollectOutputRoots(oGraph));
        for (const uint32_t uNode : oGraph.vLayoutOrder)
        {
            oGraph.vLayoutReachable[uNode] = true;
        }
    }
}
