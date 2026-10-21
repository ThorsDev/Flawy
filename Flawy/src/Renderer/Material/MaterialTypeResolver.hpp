#pragma once

#include "Renderer/Material/MaterialWorkingGraph.hpp"

#include <cstdint>
#include <span>

namespace Flawy::Renderer
{
    class MaterialTypeResolver final
    {
    public:
        void Resolve(MaterialWorkingGraph& oGraph) const;
        void ResolveLayout(MaterialWorkingGraph& oGraph) const;

    private:
        static void ResolveOrder(MaterialWorkingGraph& oGraph, std::span<const uint32_t> spanOrder);
    };
}
