#pragma once

#include "Renderer/Material/MaterialWorkingGraph.hpp"

namespace Flawy::Renderer
{
    class MaterialConstantFolder final
    {
    public:
        void Fold(MaterialWorkingGraph& oGraph) const;
    };
}
