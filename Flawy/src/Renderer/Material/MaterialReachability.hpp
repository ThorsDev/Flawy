#pragma once

#include "Renderer/Material/MaterialWorkingGraph.hpp"

namespace Flawy::Renderer
{
    class MaterialReachability final
    {
    public:
        void Mark(MaterialWorkingGraph& oGraph) const;
        void MarkLayout(MaterialWorkingGraph& oGraph) const;
    };
}
