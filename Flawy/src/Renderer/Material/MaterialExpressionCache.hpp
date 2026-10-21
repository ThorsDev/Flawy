#pragma once

#include "Renderer/Material/MaterialWorkingGraph.hpp"

namespace Flawy::Renderer
{
    class MaterialExpressionCache final
    {
    public:
        void Deduplicate(MaterialWorkingGraph& oGraph) const;
    };
}
