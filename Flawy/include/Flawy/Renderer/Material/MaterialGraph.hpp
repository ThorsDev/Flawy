#pragma once

#include "Flawy/Renderer/Material/MaterialParameterLayout.hpp"

#include <glm/vec4.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Flawy::Renderer
{
    enum class MaterialNodeKind : uint8_t
    {
        Constant,
        ScalarParameter,
        VectorParameter,
        TextureParameter,
        TextureSample,
        TexCoord,
        VertexColor,
        VertexNormal,
        Time,
        Add,
        Subtract,
        Multiply,
        Divide,
        Lerp,
        Dot,
        Saturate,
        OneMinus,
        Power,
        Normalize,
        Abs,
        Sin,
        Cos,
        ComponentMask,
        Append,
        Fresnel,
        StaticSwitch,
        CustomCode
    };

    enum class MaterialOutputSlot : uint8_t
    {
        BaseColor,
        Normal,
        Metallic,
        Roughness,
        AmbientOcclusion,
        Emissive,
        Opacity,
        WorldPositionOffset,
        Count
    };

    enum class MaterialShadingModel : uint8_t
    {
        Unlit,
        Lit
    };

    struct MaterialNodeId
    {
        static constexpr uint32_t kInvalid = 0xffffffffu;

        uint32_t uIndex = kInvalid;

        [[nodiscard]] bool IsValid() const noexcept;
        [[nodiscard]] bool operator==(const MaterialNodeId&) const noexcept = default;
    };

    struct MaterialNode
    {
        static constexpr size_t kMaxInputs = 3;

        MaterialNodeKind eKind = MaterialNodeKind::Constant;
        std::string sName;
        MaterialValueType eType = MaterialValueType::Float1;
        glm::vec4 vecValue{0.0f};
        std::array<MaterialNodeId, kMaxInputs> aInputs{};
        std::array<bool, 4> aMask{true, true, true, true};
        uint8_t uIndex = 0;
        bool bSrgb = false;
        std::string sCode;
    };

    class MaterialGraph final
    {
    public:
        static constexpr uint32_t kFormatVersion = 1;
        static constexpr size_t kOutputSlotCount = static_cast<size_t>(MaterialOutputSlot::Count);

        MaterialNodeId AddConstant(const glm::vec4& vecValue, MaterialValueType eType);
        MaterialNodeId AddScalarParameter(std::string_view svName, float fDefault);
        MaterialNodeId AddVectorParameter(std::string_view svName, const glm::vec4& vecDefault, MaterialValueType eType);
        MaterialNodeId AddTextureParameter(std::string_view svName, bool bSrgb);
        MaterialNodeId AddTextureSample(MaterialNodeId oTexture, MaterialNodeId oTexCoord);
        MaterialNodeId AddTexCoord(uint8_t uIndex);
        MaterialNodeId AddVertexColor();
        MaterialNodeId AddVertexNormal();
        MaterialNodeId AddTime();
        MaterialNodeId AddUnary(MaterialNodeKind eKind, MaterialNodeId oInput);
        MaterialNodeId AddBinary(MaterialNodeKind eKind, MaterialNodeId oLeft, MaterialNodeId oRight);
        MaterialNodeId AddLerp(MaterialNodeId oFrom, MaterialNodeId oTo, MaterialNodeId oAlpha);
        MaterialNodeId AddComponentMask(MaterialNodeId oInput, bool bR, bool bG, bool bB, bool bA);
        MaterialNodeId AddAppend(MaterialNodeId oLeft, MaterialNodeId oRight);
        MaterialNodeId AddFresnel(MaterialNodeId oExponent, float fDefaultExponent);
        MaterialNodeId AddStaticSwitch(std::string_view svName, MaterialNodeId oWhenTrue, MaterialNodeId oWhenFalse, bool bDefault);
        MaterialNodeId AddCustomCode(std::string_view svExpression, std::span<const MaterialNodeId> spanInputs, MaterialValueType eOutputType);
        void ConnectOutput(MaterialOutputSlot eSlot, MaterialNodeId oNode);
        void DisconnectOutput(MaterialOutputSlot eSlot) noexcept;
        void SetShadingModel(MaterialShadingModel eModel) noexcept;
        void SetName(std::string_view svName);

        [[nodiscard]] std::span<const MaterialNode> GetNodes() const noexcept;
        [[nodiscard]] const MaterialNode& GetNode(MaterialNodeId oId) const;
        [[nodiscard]] MaterialNodeId GetOutput(MaterialOutputSlot eSlot) const noexcept;
        [[nodiscard]] MaterialShadingModel GetShadingModel() const noexcept;
        [[nodiscard]] std::string_view GetName() const noexcept;

        [[nodiscard]] std::string ToJson() const;
        [[nodiscard]] static MaterialGraph FromJson(std::string_view svJson);

    private:
        MaterialNodeId AddNode(MaterialNode oNode);
        void RequireNode(MaterialNodeId oId) const;
        void RequireOptionalNode(MaterialNodeId oId) const;

        std::string m_sName;
        MaterialShadingModel m_eShadingModel = MaterialShadingModel::Lit;
        std::vector<MaterialNode> m_vNodes;
        std::array<MaterialNodeId, kOutputSlotCount> m_aOutputs{};
    };
}
