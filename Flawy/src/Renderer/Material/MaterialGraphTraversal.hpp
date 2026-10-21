#pragma once

#include "Flawy/Renderer/Material/MaterialGraph.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace Flawy::Renderer
{
    struct MaterialTraversalResult
    {
        std::vector<uint32_t> vPostOrder;
        std::optional<uint32_t> optCycleNode;
    };

    class MaterialGraphTraversal final
    {
    public:
        MaterialGraphTraversal() = delete;

        [[nodiscard]] static MaterialTraversalResult PostOrder(std::span<const MaterialNode> spanNodes, std::span<const uint32_t> spanRoots);
    };
}
