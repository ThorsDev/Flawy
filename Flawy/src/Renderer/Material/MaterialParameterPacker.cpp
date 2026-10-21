#include "Renderer/Material/MaterialParameterPacker.hpp"

#include <array>
#include <format>
#include <optional>
#include <utility>

namespace Flawy::Renderer
{
    namespace
    {
        constexpr uint8_t kComponentsPerSlot = 4;

        struct SlotLocation
        {
            uint8_t uSlot = 0;
            uint8_t uComponent = 0;
        };

        class SlotAllocator final
        {
        public:
            explicit SlotAllocator(const MaterialWorkingGraph& oGraph) noexcept
                : m_oGraph(oGraph)
            {
            }

            [[nodiscard]] SlotLocation Allocate(uint32_t uNode, uint8_t uComponents)
            {
                if (uComponents <= 2)
                {
                    const std::optional<SlotLocation> optExisting = FindFree(uComponents);
                    if (optExisting)
                    {
                        Claim(*optExisting, uComponents);
                        return *optExisting;
                    }
                }

                if (m_vOccupancy.size() >= MaterialParameterLayout::kMaxVec4Slots)
                {
                    m_oGraph.Fail(uNode, std::format("material parameters need more than {} vec4 slots", MaterialParameterLayout::kMaxVec4Slots));
                }
                m_vOccupancy.emplace_back();
                const SlotLocation oLocation{static_cast<uint8_t>(m_vOccupancy.size() - 1), 0};
                Claim(oLocation, uComponents);
                return oLocation;
            }

            [[nodiscard]] uint8_t GetSlotCount() const noexcept
            {
                return static_cast<uint8_t>(m_vOccupancy.size());
            }

        private:
            [[nodiscard]] std::optional<SlotLocation> FindFree(uint8_t uComponents) const noexcept
            {
                for (size_t uSlot = 0; uSlot < m_vOccupancy.size(); ++uSlot)
                {
                    for (uint8_t uStart = 0; uStart + uComponents <= kComponentsPerSlot; uStart = static_cast<uint8_t>(uStart + uComponents))
                    {
                        bool bFree = true;
                        for (uint8_t uOffset = 0; uOffset < uComponents; ++uOffset)
                        {
                            bFree = bFree && !m_vOccupancy[uSlot][uStart + uOffset];
                        }
                        if (bFree)
                        {
                            return SlotLocation{static_cast<uint8_t>(uSlot), uStart};
                        }
                    }
                }
                return std::nullopt;
            }

            void Claim(const SlotLocation& oLocation, uint8_t uComponents) noexcept
            {
                for (uint8_t uOffset = 0; uOffset < uComponents; ++uOffset)
                {
                    m_vOccupancy[oLocation.uSlot][oLocation.uComponent + uOffset] = true;
                }
            }

            const MaterialWorkingGraph& m_oGraph;
            std::vector<std::array<bool, kComponentsPerSlot>> m_vOccupancy;
        };
    }

    MaterialPackedParameters MaterialParameterPacker::Pack(const MaterialWorkingGraph& oGraph) const
    {
        MaterialPackedParameters oPacked;
        oPacked.vSlots.assign(oGraph.vNodes.size(), MaterialPackedParameters::kUnassigned);
        oPacked.vFirstComponents.assign(oGraph.vNodes.size(), MaterialPackedParameters::kUnassigned);
        oPacked.vTextureStages.assign(oGraph.vNodes.size(), MaterialPackedParameters::kUnassigned);

        SlotAllocator oAllocator(oGraph);
        uint8_t uNextStage = 0;
        for (uint32_t uNode = 0; uNode < oGraph.vNodes.size(); ++uNode)
        {
            if (!oGraph.vLayoutReachable[uNode])
            {
                continue;
            }

            const MaterialNode& oNode = oGraph.vNodes[uNode];
            if (oNode.eKind == MaterialNodeKind::TextureParameter)
            {
                if (uNextStage >= MaterialParameterLayout::kMaxTextureStages)
                {
                    oGraph.Fail(uNode, std::format("material uses more than {} textures", MaterialParameterLayout::kMaxTextureStages));
                }
                oPacked.vTextureStages[uNode] = uNextStage;
                oPacked.oLayout.vTextures.push_back({oNode.sName, uNextStage, oNode.bSrgb});
                ++uNextStage;
                continue;
            }
            if (oNode.eKind != MaterialNodeKind::ScalarParameter && oNode.eKind != MaterialNodeKind::VectorParameter)
            {
                continue;
            }

            const uint8_t uComponents = MaterialNodeTraits::GetParameterComponentCount(oNode);
            const SlotLocation oLocation = oAllocator.Allocate(uNode, uComponents);
            oPacked.vSlots[uNode] = oLocation.uSlot;
            oPacked.vFirstComponents[uNode] = oLocation.uComponent;
            oPacked.oLayout.vValues.push_back({oNode.sName, MaterialNodeTraits::FromComponentCount(uComponents), oLocation.uSlot, oLocation.uComponent, oNode.vecValue});
        }
        oPacked.oLayout.uVec4SlotCount = oAllocator.GetSlotCount();
        return oPacked;
    }
}
