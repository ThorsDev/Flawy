#pragma once

#include "Flawy/Renderer/Material/MaterialCompiler.hpp"
#include "Renderer/Material/MaterialWorkingGraph.hpp"

#include <span>
#include <vector>

namespace Flawy::Renderer
{
    class MaterialStaticSwitchResolver final
    {
    public:
        [[nodiscard]] std::vector<MaterialStaticSwitchValue> Resolve(MaterialWorkingGraph& oGraph, std::span<const MaterialStaticSwitchValue> spanValues) const;
    };
}
