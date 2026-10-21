#include "Flawy/Renderer/Material/MaterialParameterLayout.hpp"

#include <array>
#include <format>
#include <stdexcept>
#include <string_view>
#include <unordered_set>

namespace Flawy::Renderer
{
    namespace
    {
        constexpr uint8_t kComponentsPerSlot = 4;

        class ParameterNameRegistry final
        {
        public:
            void Add(std::string_view svName)
            {
                if (svName.empty())
                {
                    throw std::invalid_argument("Material parameter names must not be empty");
                }
                if (!m_setNames.insert(svName).second)
                {
                    throw std::invalid_argument(std::format("Material parameter '{}' is declared more than once", svName));
                }
            }

        private:
            std::unordered_set<std::string_view> m_setNames;
        };

        class SlotOccupancy final
        {
        public:
            void Claim(const MaterialValueParameter& oParameter)
            {
                for (uint8_t uOffset = 0; uOffset < oParameter.GetComponentCount(); ++uOffset)
                {
                    bool& bUsed = m_aUsed[oParameter.uSlot][oParameter.uComponent + uOffset];
                    if (bUsed)
                    {
                        throw std::invalid_argument(std::format("Material parameter '{}' overlaps another parameter in slot {}", oParameter.sName, oParameter.uSlot));
                    }
                    bUsed = true;
                }
            }

        private:
            std::array<std::array<bool, kComponentsPerSlot>, MaterialParameterLayout::kMaxVec4Slots> m_aUsed{};
        };
    }

    uint8_t MaterialValueParameter::GetComponentCount() const noexcept
    {
        return static_cast<uint8_t>(static_cast<uint8_t>(eType) + 1u);
    }

    void MaterialParameterLayout::Validate() const
    {
        if (uVec4SlotCount > kMaxVec4Slots)
        {
            throw std::invalid_argument(std::format("Material parameter layout uses {} vec4 slots but at most {} are allowed", uVec4SlotCount, kMaxVec4Slots));
        }

        ParameterNameRegistry oNames;
        SlotOccupancy oOccupancy;
        for (const MaterialValueParameter& oValue : vValues)
        {
            oNames.Add(oValue.sName);
            if (oValue.uSlot >= uVec4SlotCount)
            {
                throw std::invalid_argument(std::format("Material parameter '{}' uses slot {} but the layout has {} slot(s)", oValue.sName, oValue.uSlot, uVec4SlotCount));
            }
            if (oValue.uComponent + oValue.GetComponentCount() > kComponentsPerSlot)
            {
                throw std::invalid_argument(std::format("Material parameter '{}' with {} component(s) does not fit at component {}", oValue.sName, oValue.GetComponentCount(), oValue.uComponent));
            }
            oOccupancy.Claim(oValue);
        }

        std::array<bool, kMaxTextureStages> aStageUsed{};
        for (const MaterialTextureParameter& oTexture : vTextures)
        {
            oNames.Add(oTexture.sName);
            if (oTexture.uStage >= kMaxTextureStages)
            {
                throw std::invalid_argument(std::format("Material texture '{}' uses stage {} but only stages 0-{} exist", oTexture.sName, oTexture.uStage, kMaxTextureStages - 1));
            }
            if (aStageUsed[oTexture.uStage])
            {
                throw std::invalid_argument(std::format("Material texture '{}' uses stage {} which is already taken", oTexture.sName, oTexture.uStage));
            }
            aStageUsed[oTexture.uStage] = true;
        }
    }
}
