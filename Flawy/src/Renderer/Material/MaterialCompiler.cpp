#include "Flawy/Renderer/Material/MaterialCompiler.hpp"

#include "Flawy/Core/Hash.hpp"
#include "Renderer/Material/MaterialConstantFolder.hpp"
#include "Renderer/Material/MaterialExpressionCache.hpp"
#include "Renderer/Material/MaterialGraphValidator.hpp"
#include "Renderer/Material/MaterialParameterPacker.hpp"
#include "Renderer/Material/MaterialReachability.hpp"
#include "Renderer/Material/MaterialSlangEmitter.hpp"
#include "Renderer/Material/MaterialStaticSwitchResolver.hpp"
#include "Renderer/Material/MaterialText.hpp"
#include "Renderer/Material/MaterialTypeResolver.hpp"
#include "Renderer/Material/MaterialWorkingGraph.hpp"

#include <cstdint>
#include <format>
#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Flawy::Renderer
{
    namespace
    {
        constexpr std::string_view kDefaultModuleName = "Material";
        constexpr std::string_view kGeneratedModuleDirectory = "Generated/";

        std::string SanitizeIdentifier(std::string_view svName)
        {
            std::string sIdentifier;
            for (const char cCharacter : svName)
            {
                sIdentifier.push_back(MaterialText::IsAsciiLetter(cCharacter) || MaterialText::IsAsciiDigit(cCharacter) ? cCharacter : '_');
            }
            if (sIdentifier.empty())
            {
                sIdentifier = std::string(kDefaultModuleName);
            }
            if (MaterialText::IsAsciiDigit(sIdentifier.front()))
            {
                sIdentifier.insert(0, kDefaultModuleName);
            }
            return sIdentifier;
        }

        std::string BuildModuleName(const MaterialGraph& oGraph, std::span<const MaterialStaticSwitchValue> spanSwitches)
        {
            Core::Fnv1aHasher oHasher;
            oHasher.Append(oGraph.ToJson());
            oHasher.AppendValue(static_cast<uint64_t>(spanSwitches.size()));
            for (const MaterialStaticSwitchValue& oSwitch : spanSwitches)
            {
                oHasher.Append(oSwitch.sName);
                oHasher.AppendValue(oSwitch.bValue);
            }
            return std::format("{}_{:016x}", SanitizeIdentifier(oGraph.GetName()), oHasher.GetValue());
        }
    }

    MaterialCompileError::MaterialCompileError(MaterialNodeId oNode, const std::string& sMessage)
        : std::runtime_error(sMessage)
        , m_oNode(oNode)
    {
    }

    MaterialNodeId MaterialCompileError::GetNode() const noexcept
    {
        return m_oNode;
    }

    MaterialCompileResult MaterialCompiler::Compile(const MaterialGraph& oGraph, std::span<const MaterialStaticSwitchValue> spanSwitches) const
    {
        MaterialWorkingGraph oWorking(oGraph);
        MaterialGraphValidator().Validate(oWorking);
        MaterialReachability().MarkLayout(oWorking);
        MaterialTypeResolver().ResolveLayout(oWorking);
        std::vector<MaterialStaticSwitchValue> vStaticSwitches = MaterialStaticSwitchResolver().Resolve(oWorking, spanSwitches);
        MaterialReachability().Mark(oWorking);
        MaterialTypeResolver().Resolve(oWorking);
        MaterialConstantFolder().Fold(oWorking);
        MaterialExpressionCache().Deduplicate(oWorking);
        MaterialReachability().Mark(oWorking);
        MaterialPackedParameters oPacked = MaterialParameterPacker().Pack(oWorking);

        const std::string sModuleName = BuildModuleName(oGraph, vStaticSwitches);
        MaterialCompileResult oResult;
        oResult.oDescriptor.sModuleName = sModuleName;
        oResult.oDescriptor.sSourcePath = std::format("{}{}.slang", kGeneratedModuleDirectory, sModuleName);
        oResult.oDescriptor.sSlangSource = MaterialSlangEmitter().Emit(oWorking, oPacked, sModuleName);
        oResult.oDescriptor.oParameters = std::move(oPacked.oLayout);
        oResult.oDescriptor.sDebugName = oGraph.GetName().empty() ? sModuleName : std::string(oGraph.GetName());
        oResult.bUsesTangentSpaceNormal = oWorking.GetOutput(MaterialOutputSlot::Normal).IsValid();
        oResult.bUsesWorldPositionOffset = oWorking.GetOutput(MaterialOutputSlot::WorldPositionOffset).IsValid();
        oResult.eShadingModel = oWorking.eShadingModel;
        oResult.vStaticSwitches = std::move(vStaticSwitches);
        return oResult;
    }
}
