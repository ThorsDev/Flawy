#include "Renderer/Material/MaterialExpressionCache.hpp"

#include <bit>
#include <format>
#include <string>
#include <unordered_map>

namespace Flawy::Renderer
{
    namespace
    {
        std::string BuildKey(const MaterialWorkingGraph& oGraph, uint32_t uNode)
        {
            const MaterialNode& oNode = oGraph.vNodes[uNode];
            std::string sKey = std::format("{}|{}|", static_cast<int>(oNode.eKind), oGraph.vComponents[uNode]);
            for (size_t uInput = 0; uInput < oNode.aInputs.size(); ++uInput)
            {
                const MaterialNodeId oInput = oNode.aInputs[uInput];
                const bool bDefaultCoordinate = oNode.eKind == MaterialNodeKind::TextureSample && uInput == 1 &&
                                                (!oInput.IsValid() || oGraph.vNodes[oInput.uIndex].eKind == MaterialNodeKind::TexCoord);
                if (bDefaultCoordinate)
                {
                    sKey.append("uv0,");
                    continue;
                }
                sKey.append(oInput.IsValid() ? std::format("{},", oInput.uIndex) : std::string("-,"));
            }
            for (int iComponent = 0; iComponent < 4; ++iComponent)
            {
                sKey.append(std::format("{:08x},", std::bit_cast<uint32_t>(oNode.vecValue[iComponent])));
            }
            for (const bool bChannel : oNode.aMask)
            {
                sKey.push_back(bChannel ? '1' : '0');
            }
            sKey.append(std::format("|{}|{}|{}|{}|{}", static_cast<int>(oNode.eType), oNode.uIndex, oNode.bSrgb, oNode.sName.size(), oNode.sName));
            sKey.append(std::format("|{}|{}", oNode.sCode.size(), oNode.sCode));
            return sKey;
        }

        class CommonSubexpressionEliminator final
        {
        public:
            explicit CommonSubexpressionEliminator(MaterialWorkingGraph& oGraph)
                : m_oGraph(oGraph)
                , m_vCanonical(oGraph.vNodes.size(), MaterialNodeId::kInvalid)
            {
            }

            void Canonicalize(uint32_t uNode)
            {
                MaterialNode& oNode = m_oGraph.vNodes[uNode];
                const uint8_t uCount = MaterialNodeTraits::GetUsedInputCount(oNode);
                for (uint8_t uInput = 0; uInput < uCount; ++uInput)
                {
                    if (oNode.aInputs[uInput].IsValid())
                    {
                        oNode.aInputs[uInput].uIndex = m_vCanonical[oNode.aInputs[uInput].uIndex];
                    }
                }
                m_vCanonical[uNode] = m_mapExpressions.emplace(BuildKey(m_oGraph, uNode), uNode).first->second;
            }

            [[nodiscard]] uint32_t GetCanonical(uint32_t uNode) const noexcept
            {
                return m_vCanonical[uNode];
            }

        private:
            MaterialWorkingGraph& m_oGraph;
            std::vector<uint32_t> m_vCanonical;
            std::unordered_map<std::string, uint32_t> m_mapExpressions;
        };
    }

    void MaterialExpressionCache::Deduplicate(MaterialWorkingGraph& oGraph) const
    {
        CommonSubexpressionEliminator oEliminator(oGraph);
        for (const uint32_t uNode : oGraph.vOrder)
        {
            oEliminator.Canonicalize(uNode);
        }
        for (MaterialNodeId& oOutput : oGraph.aOutputs)
        {
            if (oOutput.IsValid())
            {
                oOutput.uIndex = oEliminator.GetCanonical(oOutput.uIndex);
            }
        }
    }
}
