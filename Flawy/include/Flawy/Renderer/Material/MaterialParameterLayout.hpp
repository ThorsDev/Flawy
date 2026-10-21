#pragma once

#include <glm/vec4.hpp>

#include <cstdint>
#include <string>
#include <vector>

namespace Flawy::Renderer
{
    enum class MaterialValueType : uint8_t
    {
        Float1,
        Float2,
        Float3,
        Float4
    };

    struct MaterialValueParameter
    {
        std::string sName;
        MaterialValueType eType = MaterialValueType::Float1;
        uint8_t uSlot = 0;
        uint8_t uComponent = 0;
        glm::vec4 vecDefault{0.0f};

        [[nodiscard]] uint8_t GetComponentCount() const noexcept;
    };

    struct MaterialTextureParameter
    {
        std::string sName;
        uint8_t uStage = 0;
        bool bSrgb = false;
    };

    struct MaterialParameterLayout
    {
        static constexpr uint8_t kMaxVec4Slots = 32;
        static constexpr uint8_t kMaxTextureStages = 16;

        std::vector<MaterialValueParameter> vValues;
        std::vector<MaterialTextureParameter> vTextures;
        uint8_t uVec4SlotCount = 0;

        void Validate() const;
    };
}
