#include "Renderer/Material/MaterialGraphTraversal.hpp"

#include "Renderer/Material/MaterialNodeTraits.hpp"

namespace Flawy::Renderer
{
    namespace
    {
        enum class VisitState : uint8_t
        {
            Unvisited,
            InProgress,
            Done
        };

        struct TraversalFrame
        {
            uint32_t uNode = 0;
            uint8_t uNextInput = 0;
        };
    }

    MaterialTraversalResult MaterialGraphTraversal::PostOrder(std::span<const MaterialNode> spanNodes, std::span<const uint32_t> spanRoots)
    {
        MaterialTraversalResult oResult;
        std::vector<VisitState> vStates(spanNodes.size(), VisitState::Unvisited);
        std::vector<TraversalFrame> vStack;

        for (const uint32_t uRoot : spanRoots)
        {
            if (vStates[uRoot] != VisitState::Unvisited)
            {
                continue;
            }
            vStates[uRoot] = VisitState::InProgress;
            vStack.push_back({uRoot, 0});

            while (!vStack.empty())
            {
                TraversalFrame& oFrame = vStack.back();
                const MaterialNode& oNode = spanNodes[oFrame.uNode];
                if (oFrame.uNextInput < MaterialNodeTraits::GetUsedInputCount(oNode))
                {
                    const MaterialNodeId oInput = oNode.aInputs[oFrame.uNextInput];
                    ++oFrame.uNextInput;
                    if (!oInput.IsValid())
                    {
                        continue;
                    }
                    if (vStates[oInput.uIndex] == VisitState::InProgress)
                    {
                        oResult.optCycleNode = oInput.uIndex;
                        return oResult;
                    }
                    if (vStates[oInput.uIndex] == VisitState::Unvisited)
                    {
                        vStates[oInput.uIndex] = VisitState::InProgress;
                        vStack.push_back({oInput.uIndex, 0});
                    }
                    continue;
                }

                vStates[oFrame.uNode] = VisitState::Done;
                oResult.vPostOrder.push_back(oFrame.uNode);
                vStack.pop_back();
            }
        }
        return oResult;
    }
}
