#pragma once

#include "Flawy/Renderer/Material/MaterialDescriptor.hpp"
#include "Flawy/Renderer/Material/MaterialGraph.hpp"

#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace Flawy::Renderer
{
    class MaterialCompileError final : public std::runtime_error
    {
    public:
        MaterialCompileError(MaterialNodeId oNode, const std::string& sMessage);

        [[nodiscard]] MaterialNodeId GetNode() const noexcept;

    private:
        MaterialNodeId m_oNode;
    };

    struct MaterialStaticSwitchValue
    {
        std::string sName;
        bool bValue = false;

        [[nodiscard]] bool operator==(const MaterialStaticSwitchValue&) const = default;
    };

    struct MaterialCompileResult
    {
        MaterialDescriptor oDescriptor;
        bool bUsesTangentSpaceNormal = false;
        bool bUsesWorldPositionOffset = false;
        MaterialShadingModel eShadingModel = MaterialShadingModel::Lit;
        std::vector<MaterialStaticSwitchValue> vStaticSwitches;
    };

    class MaterialCompiler final
    {
    public:
        [[nodiscard]] MaterialCompileResult Compile(const MaterialGraph& oGraph, std::span<const MaterialStaticSwitchValue> spanSwitches = {}) const;
    };
}
