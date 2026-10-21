#pragma once

#include "Renderer/Material/MaterialWorkingGraph.hpp"

namespace Flawy::Renderer
{
    class MaterialGraphValidator final
    {
    public:
        void Validate(const MaterialWorkingGraph& oGraph) const;
    };
}
