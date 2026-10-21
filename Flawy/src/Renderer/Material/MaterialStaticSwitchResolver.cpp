#include "Renderer/Material/MaterialStaticSwitchResolver.hpp"

#include <algorithm>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Flawy::Renderer
{
    namespace
    {
        constexpr uint8_t kWhenTrueInput = 0;
        constexpr uint8_t kWhenFalseInput = 1;

        class StaticSwitchSelection final
        {
        public:
            explicit StaticSwitchSelection(const MaterialWorkingGraph& oGraph)
                : m_oGraph(oGraph)
                , m_vSelected(oGraph.vNodes.size(), std::nullopt)
            {
                for (uint32_t uNode = 0; uNode < oGraph.vNodes.size(); ++uNode)
                {
                    const MaterialNode& oNode = oGraph.vNodes[uNode];
                    if (oNode.eKind == MaterialNodeKind::StaticSwitch)
                    {
                        m_mapSwitches.emplace(oNode.sName, uNode);
                        m_vSelected[uNode] = MaterialNodeTraits::GetStaticSwitchDefault(oNode);
                    }
                }
            }

            void Apply(std::span<const MaterialStaticSwitchValue> spanValues)
            {
                for (const MaterialStaticSwitchValue& oValue : spanValues)
                {
                    const auto itSwitch = m_mapSwitches.find(oValue.sName);
                    if (itSwitch == m_mapSwitches.end())
                    {
                        throw std::invalid_argument(std::format("Material graph has no static switch named '{}'", oValue.sName));
                    }
                    m_vSelected[itSwitch->second] = oValue.bValue;
                }
            }

            [[nodiscard]] uint32_t Bypass(uint32_t uNode) const noexcept
            {
                while (m_vSelected[uNode])
                {
                    const MaterialNode& oNode = m_oGraph.vNodes[uNode];
                    uNode = oNode.aInputs[*m_vSelected[uNode] ? kWhenTrueInput : kWhenFalseInput].uIndex;
                }
                return uNode;
            }

            [[nodiscard]] std::vector<MaterialStaticSwitchValue> Describe() const
            {
                std::vector<MaterialStaticSwitchValue> vValues;
                vValues.reserve(m_mapSwitches.size());
                for (const auto& [sName, uNode] : m_mapSwitches)
                {
                    vValues.push_back({sName, *m_vSelected[uNode]});
                }
                std::ranges::sort(vValues, {}, &MaterialStaticSwitchValue::sName);
                return vValues;
            }

        private:
            const MaterialWorkingGraph& m_oGraph;
            std::vector<std::optional<bool>> m_vSelected;
            std::unordered_map<std::string, uint32_t> m_mapSwitches;
        };
    }

    std::vector<MaterialStaticSwitchValue> MaterialStaticSwitchResolver::Resolve(MaterialWorkingGraph& oGraph, std::span<const MaterialStaticSwitchValue> spanValues) const
    {
        StaticSwitchSelection oSelection(oGraph);
        oSelection.Apply(spanValues);

        for (MaterialNode& oNode : oGraph.vNodes)
        {
            const uint8_t uCount = MaterialNodeTraits::GetUsedInputCount(oNode);
            for (uint8_t uInput = 0; uInput < uCount; ++uInput)
            {
                if (oNode.aInputs[uInput].IsValid())
                {
                    oNode.aInputs[uInput].uIndex = oSelection.Bypass(oNode.aInputs[uInput].uIndex);
                }
            }
        }
        for (MaterialNodeId& oOutput : oGraph.aOutputs)
        {
            if (oOutput.IsValid())
            {
                oOutput.uIndex = oSelection.Bypass(oOutput.uIndex);
            }
        }
        return oSelection.Describe();
    }
}
