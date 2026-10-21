#pragma once

#include "Renderer/Material/MaterialParameterPacker.hpp"
#include "Renderer/Material/MaterialWorkingGraph.hpp"

#include <string>
#include <string_view>

namespace Flawy::Renderer
{
    class MaterialSlangEmitter final
    {
    public:
        [[nodiscard]] std::string Emit(const MaterialWorkingGraph& oGraph, const MaterialPackedParameters& oParameters, std::string_view svModuleName) const;
    };
}
