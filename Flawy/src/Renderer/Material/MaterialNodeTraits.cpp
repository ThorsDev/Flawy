#include "Renderer/Material/MaterialNodeTraits.hpp"

#include <array>
#include <cstddef>

namespace Flawy::Renderer
{
    namespace
    {
        constexpr std::array<std::string_view, 27> kKindNames = {
            "Constant", "ScalarParameter", "VectorParameter", "TextureParameter", "TextureSample", "TexCoord", "VertexColor",
            "VertexNormal", "Time", "Add", "Subtract", "Multiply", "Divide", "Lerp", "Dot", "Saturate", "OneMinus", "Power",
            "Normalize", "Abs", "Sin", "Cos", "ComponentMask", "Append", "Fresnel", "StaticSwitch", "CustomCode",
        };

        constexpr std::array<std::string_view, MaterialGraph::kOutputSlotCount> kSlotNames = {
            "BaseColor", "Normal", "Metallic", "Roughness", "AmbientOcclusion", "Emissive", "Opacity", "WorldPositionOffset",
        };

        constexpr std::array<uint8_t, MaterialGraph::kOutputSlotCount> kSlotComponents = {3, 3, 1, 1, 1, 3, 1, 3};

        constexpr std::array<std::string_view, 4> kTypeNames = {"float1", "float2", "float3", "float4"};

        constexpr std::array<std::string_view, 2> kShadingModelNames = {"Unlit", "Lit"};

        template <typename TEnum, size_t uCount>
        std::optional<TEnum> FindByName(const std::array<std::string_view, uCount>& aNames, std::string_view svName) noexcept
        {
            for (size_t uIndex = 0; uIndex < aNames.size(); ++uIndex)
            {
                if (aNames[uIndex] == svName)
                {
                    return static_cast<TEnum>(uIndex);
                }
            }
            return std::nullopt;
        }
    }

    std::string_view MaterialNodeTraits::GetName(MaterialNodeKind eKind) noexcept
    {
        return kKindNames[static_cast<size_t>(eKind)];
    }

    std::optional<MaterialNodeKind> MaterialNodeTraits::FromName(std::string_view svName) noexcept
    {
        return FindByName<MaterialNodeKind>(kKindNames, svName);
    }

    MaterialNodeSignature MaterialNodeTraits::GetSignature(MaterialNodeKind eKind) noexcept
    {
        switch (eKind)
        {
        case MaterialNodeKind::TextureSample:
            return {1, 2};
        case MaterialNodeKind::Add:
        case MaterialNodeKind::Subtract:
        case MaterialNodeKind::Multiply:
        case MaterialNodeKind::Divide:
        case MaterialNodeKind::Dot:
        case MaterialNodeKind::Power:
        case MaterialNodeKind::Append:
        case MaterialNodeKind::StaticSwitch:
            return {2, 2};
        case MaterialNodeKind::Lerp:
            return {3, 3};
        case MaterialNodeKind::Saturate:
        case MaterialNodeKind::OneMinus:
        case MaterialNodeKind::Normalize:
        case MaterialNodeKind::Abs:
        case MaterialNodeKind::Sin:
        case MaterialNodeKind::Cos:
        case MaterialNodeKind::ComponentMask:
            return {1, 1};
        case MaterialNodeKind::Fresnel:
            return {0, 1};
        case MaterialNodeKind::CustomCode:
            return {0, 3};
        case MaterialNodeKind::Constant:
        case MaterialNodeKind::ScalarParameter:
        case MaterialNodeKind::VectorParameter:
        case MaterialNodeKind::TextureParameter:
        case MaterialNodeKind::TexCoord:
        case MaterialNodeKind::VertexColor:
        case MaterialNodeKind::VertexNormal:
        case MaterialNodeKind::Time:
            break;
        }
        return {0, 0};
    }

    uint8_t MaterialNodeTraits::GetUsedInputCount(const MaterialNode& oNode) noexcept
    {
        if (oNode.eKind != MaterialNodeKind::CustomCode)
        {
            return GetSignature(oNode.eKind).uMaxInputs;
        }

        uint8_t uCount = 0;
        while (uCount < MaterialNode::kMaxInputs && oNode.aInputs[uCount].IsValid())
        {
            ++uCount;
        }
        return uCount;
    }

    bool MaterialNodeTraits::IsUnary(MaterialNodeKind eKind) noexcept
    {
        switch (eKind)
        {
        case MaterialNodeKind::Saturate:
        case MaterialNodeKind::OneMinus:
        case MaterialNodeKind::Normalize:
        case MaterialNodeKind::Abs:
        case MaterialNodeKind::Sin:
        case MaterialNodeKind::Cos:
            return true;
        default:
            return false;
        }
    }

    bool MaterialNodeTraits::IsBinary(MaterialNodeKind eKind) noexcept
    {
        switch (eKind)
        {
        case MaterialNodeKind::Add:
        case MaterialNodeKind::Subtract:
        case MaterialNodeKind::Multiply:
        case MaterialNodeKind::Divide:
        case MaterialNodeKind::Dot:
        case MaterialNodeKind::Power:
            return true;
        default:
            return false;
        }
    }

    bool MaterialNodeTraits::IsParameter(MaterialNodeKind eKind) noexcept
    {
        return eKind == MaterialNodeKind::ScalarParameter || eKind == MaterialNodeKind::VectorParameter || eKind == MaterialNodeKind::TextureParameter;
    }

    uint8_t MaterialNodeTraits::GetParameterComponentCount(const MaterialNode& oNode) noexcept
    {
        return oNode.eKind == MaterialNodeKind::VectorParameter ? GetComponentCount(oNode.eType) : uint8_t{1};
    }

    bool MaterialNodeTraits::GetStaticSwitchDefault(const MaterialNode& oNode) noexcept
    {
        return oNode.vecValue.x != 0.0f;
    }

    glm::vec4 MaterialNodeTraits::MakeStaticSwitchValue(bool bValue) noexcept
    {
        return glm::vec4(bValue ? 1.0f : 0.0f, 0.0f, 0.0f, 0.0f);
    }

    std::string_view MaterialNodeTraits::GetSlotName(MaterialOutputSlot eSlot) noexcept
    {
        return kSlotNames[static_cast<size_t>(eSlot)];
    }

    std::optional<MaterialOutputSlot> MaterialNodeTraits::SlotFromName(std::string_view svName) noexcept
    {
        return FindByName<MaterialOutputSlot>(kSlotNames, svName);
    }

    uint8_t MaterialNodeTraits::GetSlotComponentCount(MaterialOutputSlot eSlot) noexcept
    {
        return kSlotComponents[static_cast<size_t>(eSlot)];
    }

    std::string_view MaterialNodeTraits::GetTypeName(MaterialValueType eType) noexcept
    {
        return kTypeNames[static_cast<size_t>(eType)];
    }

    std::optional<MaterialValueType> MaterialNodeTraits::TypeFromName(std::string_view svName) noexcept
    {
        return FindByName<MaterialValueType>(kTypeNames, svName);
    }

    uint8_t MaterialNodeTraits::GetComponentCount(MaterialValueType eType) noexcept
    {
        return static_cast<uint8_t>(static_cast<uint8_t>(eType) + 1u);
    }

    MaterialValueType MaterialNodeTraits::FromComponentCount(uint8_t uCount) noexcept
    {
        return static_cast<MaterialValueType>(uCount - 1u);
    }

    std::string_view MaterialNodeTraits::GetShadingModelName(MaterialShadingModel eModel) noexcept
    {
        return kShadingModelNames[static_cast<size_t>(eModel)];
    }

    std::optional<MaterialShadingModel> MaterialNodeTraits::ShadingModelFromName(std::string_view svName) noexcept
    {
        return FindByName<MaterialShadingModel>(kShadingModelNames, svName);
    }
}
