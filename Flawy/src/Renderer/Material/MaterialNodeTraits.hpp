#pragma once

#include "Flawy/Renderer/Material/MaterialGraph.hpp"

#include <glm/vec4.hpp>

#include <cstdint>
#include <optional>
#include <string_view>

namespace Flawy::Renderer
{
    struct MaterialNodeSignature
    {
        uint8_t uRequiredInputs = 0;
        uint8_t uMaxInputs = 0;
    };

    class MaterialNodeTraits final
    {
    public:
        MaterialNodeTraits() = delete;

        [[nodiscard]] static std::string_view GetName(MaterialNodeKind eKind) noexcept;
        [[nodiscard]] static std::optional<MaterialNodeKind> FromName(std::string_view svName) noexcept;
        [[nodiscard]] static MaterialNodeSignature GetSignature(MaterialNodeKind eKind) noexcept;
        [[nodiscard]] static uint8_t GetUsedInputCount(const MaterialNode& oNode) noexcept;
        [[nodiscard]] static bool IsUnary(MaterialNodeKind eKind) noexcept;
        [[nodiscard]] static bool IsBinary(MaterialNodeKind eKind) noexcept;
        [[nodiscard]] static bool IsParameter(MaterialNodeKind eKind) noexcept;
        [[nodiscard]] static uint8_t GetParameterComponentCount(const MaterialNode& oNode) noexcept;
        [[nodiscard]] static bool GetStaticSwitchDefault(const MaterialNode& oNode) noexcept;
        [[nodiscard]] static glm::vec4 MakeStaticSwitchValue(bool bValue) noexcept;

        [[nodiscard]] static std::string_view GetSlotName(MaterialOutputSlot eSlot) noexcept;
        [[nodiscard]] static std::optional<MaterialOutputSlot> SlotFromName(std::string_view svName) noexcept;
        [[nodiscard]] static uint8_t GetSlotComponentCount(MaterialOutputSlot eSlot) noexcept;

        [[nodiscard]] static std::string_view GetTypeName(MaterialValueType eType) noexcept;
        [[nodiscard]] static std::optional<MaterialValueType> TypeFromName(std::string_view svName) noexcept;
        [[nodiscard]] static uint8_t GetComponentCount(MaterialValueType eType) noexcept;
        [[nodiscard]] static MaterialValueType FromComponentCount(uint8_t uCount) noexcept;

        [[nodiscard]] static std::string_view GetShadingModelName(MaterialShadingModel eModel) noexcept;
        [[nodiscard]] static std::optional<MaterialShadingModel> ShadingModelFromName(std::string_view svName) noexcept;
    };
}
