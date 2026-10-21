#include "Flawy/Renderer/Material/MaterialGraph.hpp"

#include "Renderer/Material/JsonWriter.hpp"
#include "Renderer/Material/MaterialNodeTraits.hpp"

#include <simdjson.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <format>
#include <initializer_list>
#include <iterator>
#include <optional>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace Flawy::Renderer
{
    namespace
    {
        constexpr std::string_view kMaskChannels = "rgba";

        enum class NodeField : uint8_t
        {
            Name,
            Type,
            Value,
            Srgb,
            Inputs,
            Index,
            Mask,
            Code,
            Default,
            Count
        };

        constexpr std::array<std::string_view, static_cast<size_t>(NodeField::Count)> kFieldNames = {"name", "type", "value", "srgb", "inputs", "index", "mask", "code", "default"};

        class NodeFieldSet final
        {
        public:
            explicit NodeFieldSet(MaterialNodeKind eKind) noexcept
            {
                switch (eKind)
                {
                case MaterialNodeKind::Constant:
                    Enable({NodeField::Type, NodeField::Value});
                    break;
                case MaterialNodeKind::ScalarParameter:
                    Enable({NodeField::Name, NodeField::Value});
                    break;
                case MaterialNodeKind::VectorParameter:
                    Enable({NodeField::Name, NodeField::Type, NodeField::Value});
                    break;
                case MaterialNodeKind::TextureParameter:
                    Enable({NodeField::Name, NodeField::Srgb});
                    break;
                case MaterialNodeKind::TexCoord:
                    Enable({NodeField::Index});
                    break;
                case MaterialNodeKind::VertexColor:
                case MaterialNodeKind::VertexNormal:
                case MaterialNodeKind::Time:
                    break;
                case MaterialNodeKind::ComponentMask:
                    Enable({NodeField::Inputs, NodeField::Mask});
                    break;
                case MaterialNodeKind::Fresnel:
                    Enable({NodeField::Inputs, NodeField::Value});
                    break;
                case MaterialNodeKind::StaticSwitch:
                    Enable({NodeField::Name, NodeField::Default, NodeField::Inputs});
                    break;
                case MaterialNodeKind::CustomCode:
                    Enable({NodeField::Type, NodeField::Code, NodeField::Inputs});
                    break;
                default:
                    Enable({NodeField::Inputs});
                    break;
                }
            }

            [[nodiscard]] bool Has(NodeField eField) const noexcept
            {
                return m_aEnabled[static_cast<size_t>(eField)];
            }

        private:
            void Enable(std::initializer_list<NodeField> oFields) noexcept
            {
                for (const NodeField eField : oFields)
                {
                    m_aEnabled[static_cast<size_t>(eField)] = true;
                }
            }

            std::array<bool, static_cast<size_t>(NodeField::Count)> m_aEnabled{};
        };

        std::string ToMaskText(const std::array<bool, 4>& aMask)
        {
            std::string sMask;
            for (size_t uChannel = 0; uChannel < aMask.size(); ++uChannel)
            {
                if (aMask[uChannel])
                {
                    sMask.push_back(kMaskChannels[uChannel]);
                }
            }
            return sMask;
        }

        void WriteNode(JsonWriter& oWriter, const MaterialNode& oNode, size_t uId)
        {
            const NodeFieldSet oFields(oNode.eKind);
            oWriter.BeginObject();
            oWriter.Key("id");
            oWriter.Integer(static_cast<int64_t>(uId));
            oWriter.Key("kind");
            oWriter.String(MaterialNodeTraits::GetName(oNode.eKind));
            if (oFields.Has(NodeField::Name))
            {
                oWriter.Key("name");
                oWriter.String(oNode.sName);
            }
            if (oFields.Has(NodeField::Type))
            {
                oWriter.Key("type");
                oWriter.String(MaterialNodeTraits::GetTypeName(oNode.eType));
            }
            if (oFields.Has(NodeField::Value))
            {
                oWriter.Key("value");
                oWriter.BeginArray();
                for (int iComponent = 0; iComponent < 4; ++iComponent)
                {
                    oWriter.Number(oNode.vecValue[iComponent]);
                }
                oWriter.EndArray();
            }
            if (oFields.Has(NodeField::Srgb))
            {
                oWriter.Key("srgb");
                oWriter.Bool(oNode.bSrgb);
            }
            if (oFields.Has(NodeField::Index))
            {
                oWriter.Key("index");
                oWriter.Integer(oNode.uIndex);
            }
            if (oFields.Has(NodeField::Mask))
            {
                oWriter.Key("mask");
                oWriter.String(ToMaskText(oNode.aMask));
            }
            if (oFields.Has(NodeField::Code))
            {
                oWriter.Key("code");
                oWriter.String(oNode.sCode);
            }
            if (oFields.Has(NodeField::Default))
            {
                oWriter.Key("default");
                oWriter.Bool(MaterialNodeTraits::GetStaticSwitchDefault(oNode));
            }
            if (oFields.Has(NodeField::Inputs))
            {
                oWriter.Key("inputs");
                oWriter.BeginArray();
                const uint8_t uCount = MaterialNodeTraits::GetUsedInputCount(oNode);
                for (uint8_t uInput = 0; uInput < uCount; ++uInput)
                {
                    const MaterialNodeId oInput = oNode.aInputs[uInput];
                    oWriter.Integer(oInput.IsValid() ? static_cast<int64_t>(oInput.uIndex) : -1);
                }
                oWriter.EndArray();
            }
            oWriter.EndObject();
        }

        class JsonGraphReader final
        {
        public:
            [[noreturn]] static void Fail(std::string_view svPath, std::string_view svReason)
            {
                throw std::invalid_argument(std::format("Material graph JSON {}: {}", svPath, svReason));
            }

            static simdjson::dom::object RequireObject(simdjson::dom::element oElement, std::string_view svPath)
            {
                simdjson::dom::object oObject;
                if (oElement.get_object().get(oObject) != simdjson::SUCCESS)
                {
                    Fail(svPath, "expected an object");
                }

                std::unordered_set<std::string_view> setKeys;
                for (const simdjson::dom::key_value_pair oField : oObject)
                {
                    if (!setKeys.insert(oField.key).second)
                    {
                        const std::string sKeyPath = svPath == "document" ? std::string(oField.key) : std::format("{}.{}", svPath, oField.key);
                        Fail(sKeyPath, "duplicate key");
                    }
                }
                return oObject;
            }

            static simdjson::dom::array RequireArray(simdjson::dom::element oElement, std::string_view svPath)
            {
                simdjson::dom::array oArray;
                if (oElement.get_array().get(oArray) != simdjson::SUCCESS)
                {
                    Fail(svPath, "expected an array");
                }
                return oArray;
            }

            static std::string_view RequireString(simdjson::dom::element oElement, std::string_view svPath)
            {
                std::string_view svValue;
                if (oElement.get_string().get(svValue) != simdjson::SUCCESS)
                {
                    Fail(svPath, "expected a string");
                }
                return svValue;
            }

            static int64_t RequireInteger(simdjson::dom::element oElement, std::string_view svPath)
            {
                int64_t iValue = 0;
                if (oElement.get_int64().get(iValue) != simdjson::SUCCESS)
                {
                    Fail(svPath, "expected an integer");
                }
                return iValue;
            }

            static float RequireNumber(simdjson::dom::element oElement, std::string_view svPath)
            {
                double dValue = 0.0;
                if (oElement.get_double().get(dValue) != simdjson::SUCCESS)
                {
                    Fail(svPath, "expected a number");
                }
                const auto fValue = static_cast<float>(dValue);
                if (!std::isfinite(fValue))
                {
                    Fail(svPath, "number is not representable as a finite float");
                }
                return fValue;
            }

            static bool RequireBool(simdjson::dom::element oElement, std::string_view svPath)
            {
                bool bValue = false;
                if (oElement.get_bool().get(bValue) != simdjson::SUCCESS)
                {
                    Fail(svPath, "expected a boolean");
                }
                return bValue;
            }

            static simdjson::dom::element RequireField(simdjson::dom::object oObject, std::string_view svKey, std::string_view svPath)
            {
                simdjson::dom::element oElement;
                if (oObject.at_key(svKey).get(oElement) != simdjson::SUCCESS)
                {
                    Fail(svPath, std::format("missing field '{}'", svKey));
                }
                return oElement;
            }

            static MaterialNode ReadNode(simdjson::dom::element oElement, size_t uExpectedId, size_t uNodeCount)
            {
                const std::string sPath = std::format("nodes[{}]", uExpectedId);
                const simdjson::dom::object oObject = RequireObject(oElement, sPath);

                const int64_t iId = RequireInteger(RequireField(oObject, "id", sPath), sPath + ".id");
                if (iId < 0 || static_cast<size_t>(iId) != uExpectedId)
                {
                    Fail(sPath + ".id", std::format("expected {} (node ids are array positions)", uExpectedId));
                }

                const std::string_view svKind = RequireString(RequireField(oObject, "kind", sPath), sPath + ".kind");
                const std::optional<MaterialNodeKind> optKind = MaterialNodeTraits::FromName(svKind);
                if (!optKind)
                {
                    Fail(sPath + ".kind", std::format("unknown node kind '{}'", svKind));
                }

                MaterialNode oNode;
                oNode.eKind = *optKind;
                const NodeFieldSet oFields(oNode.eKind);
                for (const auto [svKey, oValue] : oObject)
                {
                    const std::string sFieldPath = std::format("{}.{}", sPath, svKey);
                    if (svKey == "id" || svKey == "kind")
                    {
                        continue;
                    }
                    const auto itField = std::ranges::find(kFieldNames, svKey);
                    if (itField == kFieldNames.end() || !oFields.Has(static_cast<NodeField>(std::distance(kFieldNames.begin(), itField))))
                    {
                        Fail(sFieldPath, std::format("unexpected field for a {} node", svKind));
                    }
                }

                if (oFields.Has(NodeField::Name))
                {
                    oNode.sName = std::string(RequireString(RequireField(oObject, "name", sPath), sPath + ".name"));
                    if (oNode.sName.empty())
                    {
                        Fail(sPath + ".name", "must not be empty");
                    }
                }
                if (oFields.Has(NodeField::Type))
                {
                    const std::string_view svType = RequireString(RequireField(oObject, "type", sPath), sPath + ".type");
                    const std::optional<MaterialValueType> optType = MaterialNodeTraits::TypeFromName(svType);
                    if (!optType)
                    {
                        Fail(sPath + ".type", std::format("unknown value type '{}'", svType));
                    }
                    oNode.eType = *optType;
                }
                if (oFields.Has(NodeField::Value))
                {
                    oNode.vecValue = ReadVec4(RequireField(oObject, "value", sPath), sPath + ".value");
                }
                if (oFields.Has(NodeField::Srgb))
                {
                    oNode.bSrgb = RequireBool(RequireField(oObject, "srgb", sPath), sPath + ".srgb");
                }
                if (oFields.Has(NodeField::Index))
                {
                    const int64_t iIndex = RequireInteger(RequireField(oObject, "index", sPath), sPath + ".index");
                    if (iIndex != 0)
                    {
                        Fail(sPath + ".index", "only TEXCOORD0 is supported");
                    }
                    oNode.uIndex = 0;
                }
                if (oFields.Has(NodeField::Mask))
                {
                    oNode.aMask = ReadMask(RequireString(RequireField(oObject, "mask", sPath), sPath + ".mask"), sPath + ".mask");
                }
                if (oFields.Has(NodeField::Code))
                {
                    oNode.sCode = std::string(RequireString(RequireField(oObject, "code", sPath), sPath + ".code"));
                }
                if (oFields.Has(NodeField::Default))
                {
                    oNode.vecValue = MaterialNodeTraits::MakeStaticSwitchValue(RequireBool(RequireField(oObject, "default", sPath), sPath + ".default"));
                }
                if (oFields.Has(NodeField::Inputs))
                {
                    ReadInputs(oNode, RequireField(oObject, "inputs", sPath), sPath + ".inputs", uNodeCount);
                }
                return oNode;
            }

        private:
            static glm::vec4 ReadVec4(simdjson::dom::element oElement, const std::string& sPath)
            {
                const simdjson::dom::array oArray = RequireArray(oElement, sPath);
                if (oArray.size() != 4)
                {
                    Fail(sPath, "expected exactly 4 numbers");
                }
                glm::vec4 vecValue(0.0f);
                int iComponent = 0;
                for (const simdjson::dom::element oNumber : oArray)
                {
                    vecValue[iComponent] = RequireNumber(oNumber, std::format("{}[{}]", sPath, iComponent));
                    ++iComponent;
                }
                return vecValue;
            }

            static std::array<bool, 4> ReadMask(std::string_view svMask, const std::string& sPath)
            {
                std::array<bool, 4> aMask{};
                size_t uSearchFrom = 0;
                for (const char cChannel : svMask)
                {
                    const size_t uChannel = kMaskChannels.find(cChannel, uSearchFrom);
                    if (uChannel == std::string_view::npos)
                    {
                        Fail(sPath, "must list distinct channels of 'rgba' in order");
                    }
                    aMask[uChannel] = true;
                    uSearchFrom = uChannel + 1;
                }
                if (svMask.empty())
                {
                    Fail(sPath, "must keep at least one channel");
                }
                return aMask;
            }

            static void ReadInputs(MaterialNode& oNode, simdjson::dom::element oElement, const std::string& sPath, size_t uNodeCount)
            {
                const simdjson::dom::array oArray = RequireArray(oElement, sPath);
                const MaterialNodeSignature oSignature = MaterialNodeTraits::GetSignature(oNode.eKind);
                const bool bVariable = oNode.eKind == MaterialNodeKind::CustomCode;
                if ((!bVariable && oArray.size() != oSignature.uMaxInputs) || (bVariable && oArray.size() > oSignature.uMaxInputs))
                {
                    Fail(sPath, std::format("{} nodes take {} input(s)", MaterialNodeTraits::GetName(oNode.eKind), oSignature.uMaxInputs));
                }

                size_t uInput = 0;
                for (const simdjson::dom::element oId : oArray)
                {
                    const std::string sInputPath = std::format("{}[{}]", sPath, uInput);
                    const int64_t iId = RequireInteger(oId, sInputPath);
                    if (iId == -1)
                    {
                        if (bVariable)
                        {
                            Fail(sInputPath, "CustomCode inputs must all be connected");
                        }
                    }
                    else if (iId < 0 || static_cast<uint64_t>(iId) >= uNodeCount)
                    {
                        Fail(sInputPath, std::format("node id {} does not exist", iId));
                    }
                    else
                    {
                        oNode.aInputs[uInput] = MaterialNodeId{static_cast<uint32_t>(iId)};
                    }
                    ++uInput;
                }
            }
        };
    }

    std::string MaterialGraph::ToJson() const
    {
        JsonWriter oWriter;
        oWriter.BeginObject();
        oWriter.Key("version");
        oWriter.Integer(kFormatVersion);
        oWriter.Key("name");
        oWriter.String(m_sName);
        oWriter.Key("shadingModel");
        oWriter.String(MaterialNodeTraits::GetShadingModelName(m_eShadingModel));
        oWriter.Key("nodes");
        oWriter.BeginArray();
        for (size_t uIndex = 0; uIndex < m_vNodes.size(); ++uIndex)
        {
            WriteNode(oWriter, m_vNodes[uIndex], uIndex);
        }
        oWriter.EndArray();
        oWriter.Key("outputs");
        oWriter.BeginObject();
        for (size_t uSlot = 0; uSlot < kOutputSlotCount; ++uSlot)
        {
            if (m_aOutputs[uSlot].IsValid())
            {
                oWriter.Key(MaterialNodeTraits::GetSlotName(static_cast<MaterialOutputSlot>(uSlot)));
                oWriter.Integer(m_aOutputs[uSlot].uIndex);
            }
        }
        oWriter.EndObject();
        oWriter.EndObject();
        return oWriter.GetText();
    }

    MaterialGraph MaterialGraph::FromJson(std::string_view svJson)
    {
        simdjson::dom::parser oParser;
        const simdjson::padded_string oPadded(svJson);
        simdjson::dom::element oRoot;
        const simdjson::error_code eError = oParser.parse(oPadded).get(oRoot);
        if (eError != simdjson::SUCCESS)
        {
            JsonGraphReader::Fail("document", simdjson::error_message(eError));
        }

        const simdjson::dom::object oObject = JsonGraphReader::RequireObject(oRoot, "document");
        for (const auto [svKey, oValue] : oObject)
        {
            if (svKey != "version" && svKey != "name" && svKey != "shadingModel" && svKey != "nodes" && svKey != "outputs")
            {
                JsonGraphReader::Fail(svKey, "unexpected top-level field");
            }
        }

        const int64_t iVersion = JsonGraphReader::RequireInteger(JsonGraphReader::RequireField(oObject, "version", "document"), "version");
        if (iVersion != kFormatVersion)
        {
            JsonGraphReader::Fail("version", std::format("unsupported version {} (expected {})", iVersion, kFormatVersion));
        }

        MaterialGraph oGraph;
        oGraph.m_sName = std::string(JsonGraphReader::RequireString(JsonGraphReader::RequireField(oObject, "name", "document"), "name"));

        const std::string_view svModel = JsonGraphReader::RequireString(JsonGraphReader::RequireField(oObject, "shadingModel", "document"), "shadingModel");
        const std::optional<MaterialShadingModel> optModel = MaterialNodeTraits::ShadingModelFromName(svModel);
        if (!optModel)
        {
            JsonGraphReader::Fail("shadingModel", std::format("unknown shading model '{}'", svModel));
        }
        oGraph.m_eShadingModel = *optModel;

        const simdjson::dom::array oNodes = JsonGraphReader::RequireArray(JsonGraphReader::RequireField(oObject, "nodes", "document"), "nodes");
        const size_t uNodeCount = oNodes.size();
        oGraph.m_vNodes.reserve(uNodeCount);
        for (const simdjson::dom::element oNode : oNodes)
        {
            oGraph.m_vNodes.push_back(JsonGraphReader::ReadNode(oNode, oGraph.m_vNodes.size(), uNodeCount));
        }

        const simdjson::dom::object oOutputs = JsonGraphReader::RequireObject(JsonGraphReader::RequireField(oObject, "outputs", "document"), "outputs");
        for (const auto [svKey, oValue] : oOutputs)
        {
            const std::string sPath = std::format("outputs.{}", svKey);
            const std::optional<MaterialOutputSlot> optSlot = MaterialNodeTraits::SlotFromName(svKey);
            if (!optSlot)
            {
                JsonGraphReader::Fail(sPath, "unknown output slot");
            }
            const int64_t iId = JsonGraphReader::RequireInteger(oValue, sPath);
            if (iId < 0 || static_cast<uint64_t>(iId) >= uNodeCount)
            {
                JsonGraphReader::Fail(sPath, std::format("node id {} does not exist", iId));
            }
            oGraph.m_aOutputs[static_cast<size_t>(*optSlot)] = MaterialNodeId{static_cast<uint32_t>(iId)};
        }
        return oGraph;
    }
}
