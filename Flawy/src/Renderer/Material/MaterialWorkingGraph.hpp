#pragma once

#include "Flawy/Renderer/Material/MaterialCompiler.hpp"
#include "Flawy/Renderer/Material/MaterialGraph.hpp"
#include "Renderer/Material/MaterialNodeTraits.hpp"

#include <array>
#include <cstdint>
#include <format>
#include <string>
#include <vector>

namespace Flawy::Renderer
{
    struct MaterialWorkingGraph
    {
        static constexpr uint8_t kTextureComponents = 0;
        static constexpr uint32_t kMaxNodeCount = 65536;

        explicit MaterialWorkingGraph(const MaterialGraph& oGraph)
            : vNodes(oGraph.GetNodes().begin(), oGraph.GetNodes().end())
            , vComponents(vNodes.size(), kTextureComponents)
            , vReachable(vNodes.size(), false)
            , vLayoutReachable(vNodes.size(), false)
            , vVertexStage(vNodes.size(), false)
            , eShadingModel(oGraph.GetShadingModel())
        {
            for (size_t uSlot = 0; uSlot < MaterialGraph::kOutputSlotCount; ++uSlot)
            {
                aOutputs[uSlot] = oGraph.GetOutput(static_cast<MaterialOutputSlot>(uSlot));
            }
        }

        [[nodiscard]] MaterialNodeId GetOutput(MaterialOutputSlot eSlot) const noexcept
        {
            return aOutputs[static_cast<size_t>(eSlot)];
        }

        [[nodiscard]] std::string Describe(uint32_t uNode) const
        {
            return std::format("node {} ({})", uNode, MaterialNodeTraits::GetName(vNodes[uNode].eKind));
        }

        [[noreturn]] void Fail(uint32_t uNode, std::string_view svReason) const
        {
            throw MaterialCompileError(MaterialNodeId{uNode}, std::format("Material graph {}: {}", Describe(uNode), svReason));
        }

        std::vector<MaterialNode> vNodes;
        std::vector<uint8_t> vComponents;
        std::vector<bool> vReachable;
        std::vector<bool> vLayoutReachable;
        std::vector<bool> vVertexStage;
        std::vector<uint32_t> vOrder;
        std::vector<uint32_t> vLayoutOrder;
        std::array<MaterialNodeId, MaterialGraph::kOutputSlotCount> aOutputs{};
        MaterialShadingModel eShadingModel = MaterialShadingModel::Lit;
    };
}
