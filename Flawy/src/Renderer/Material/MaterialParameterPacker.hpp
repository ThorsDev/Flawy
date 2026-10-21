#pragma once

#include "Flawy/Renderer/Material/MaterialParameterLayout.hpp"
#include "Renderer/Material/MaterialWorkingGraph.hpp"

#include <cstdint>
#include <vector>

namespace Flawy::Renderer
{
    struct MaterialPackedParameters
    {
        static constexpr uint8_t kUnassigned = 0xff;

        MaterialParameterLayout oLayout;
        std::vector<uint8_t> vSlots;
        std::vector<uint8_t> vFirstComponents;
        std::vector<uint8_t> vTextureStages;
    };

    class MaterialParameterPacker final
    {
    public:
        [[nodiscard]] MaterialPackedParameters Pack(const MaterialWorkingGraph& oGraph) const;
    };
}
