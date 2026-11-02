#include "Flawy/Animation/AnimationControllerDescriptor.hpp"

#include "Animation/Json/AnimationJsonReader.hpp"

#include <format>
#include <optional>
#include <string>
#include <utility>

namespace Flawy::Animation
{
    namespace
    {
        using Json = AnimationJsonReader;

        class ControllerJsonParser final
        {
        public:
            [[nodiscard]] static AnimationControllerDescriptor Parse(simdjson::dom::element oRoot)
            {
                const simdjson::dom::object oObject =
                    Json::RequireObject(oRoot, "document", {"version", "name", "parameters", "entry", "states", "transitions"});

                const int64_t iVersion = Json::RequireInteger(Json::RequireField(oObject, "version", "document"), "version");
                if (iVersion != AnimationControllerDescriptor::kFormatVersion)
                {
                    Json::Fail("version", std::format("unsupported version {} (expected {})",
                                                      iVersion,
                                                      AnimationControllerDescriptor::kFormatVersion));
                }

                AnimationControllerDescriptor oDescriptor;
                oDescriptor.sName = Json::RequireNameField(oObject, "name", "document");
                oDescriptor.sEntryState = Json::RequireNameField(oObject, "entry", "document");

                if (const std::optional<simdjson::dom::element> optParameters = Json::FindField(oObject, "parameters"))
                {
                    size_t uIndex = 0;
                    for (const simdjson::dom::element oParameter : Json::RequireArray(*optParameters, "parameters"))
                    {
                        oDescriptor.vParameters.push_back(ReadParameter(oParameter, Json::IndexPath("parameters", uIndex++)));
                    }
                }

                size_t uStateIndex = 0;
                for (const simdjson::dom::element oState : Json::RequireArray(Json::RequireField(oObject, "states", "document"), "states"))
                {
                    oDescriptor.vStates.push_back(ReadState(oState, Json::IndexPath("states", uStateIndex++)));
                }

                if (const std::optional<simdjson::dom::element> optTransitions = Json::FindField(oObject, "transitions"))
                {
                    size_t uIndex = 0;
                    for (const simdjson::dom::element oTransition : Json::RequireArray(*optTransitions, "transitions"))
                    {
                        oDescriptor.vTransitions.push_back(ReadTransition(oTransition, Json::IndexPath("transitions", uIndex++)));
                    }
                }
                return oDescriptor;
            }

        private:
            static AnimationParameterDescriptor ReadParameter(simdjson::dom::element oElement, const std::string& sPath)
            {
                const simdjson::dom::object oObject = Json::RequireObject(oElement, sPath, {"name", "type", "default"});

                AnimationParameterDescriptor oParameter;
                oParameter.sName = Json::RequireNameField(oObject, "name", sPath);

                const std::string sTypePath = Json::JoinPath(sPath, "type");
                const std::string_view svType = Json::RequireString(Json::RequireField(oObject, "type", sPath), sTypePath);
                const std::optional<simdjson::dom::element> optDefault = Json::FindField(oObject, "default");
                const std::string sDefaultPath = Json::JoinPath(sPath, "default");
                if (svType == "float")
                {
                    oParameter.eType = AnimationParameterType::Float;
                    oParameter.fDefaultValue = optDefault ? Json::RequireNumber(*optDefault, sDefaultPath) : 0.0f;
                }
                else if (svType == "bool")
                {
                    oParameter.eType = AnimationParameterType::Bool;
                    oParameter.bDefaultValue = optDefault ? Json::RequireBool(*optDefault, sDefaultPath) : false;
                }
                else if (svType == "trigger")
                {
                    oParameter.eType = AnimationParameterType::Trigger;
                    if (optDefault)
                    {
                        Json::Fail(sDefaultPath, "trigger parameters have no default value");
                    }
                }
                else
                {
                    Json::Fail(sTypePath, std::format("unknown parameter type '{}' (expected float, bool or trigger)", svType));
                }
                return oParameter;
            }

            static AnimationStateDescriptor ReadState(simdjson::dom::element oElement, const std::string& sPath)
            {
                const simdjson::dom::object oObject = Json::RequireObject(oElement, sPath, {"name", "clip", "blend", "speed", "loop"});

                AnimationStateDescriptor oState;
                oState.sName = Json::RequireNameField(oObject, "name", sPath);

                const std::optional<simdjson::dom::element> optClip = Json::FindField(oObject, "clip");
                const std::optional<simdjson::dom::element> optBlend = Json::FindField(oObject, "blend");
                if (optClip.has_value() == optBlend.has_value())
                {
                    Json::Fail(sPath, "a state must have exactly one of 'clip' or 'blend'");
                }
                if (optClip)
                {
                    oState.sClipName = Json::RequireName(*optClip, Json::JoinPath(sPath, "clip"));
                }
                else
                {
                    ReadBlendSpace(oState, *optBlend, Json::JoinPath(sPath, "blend"));
                }

                if (const std::optional<simdjson::dom::element> optSpeed = Json::FindField(oObject, "speed"))
                {
                    oState.fSpeed = Json::RequireNumber(*optSpeed, Json::JoinPath(sPath, "speed"));
                }
                if (const std::optional<simdjson::dom::element> optLoop = Json::FindField(oObject, "loop"))
                {
                    oState.bLoop = Json::RequireBool(*optLoop, Json::JoinPath(sPath, "loop"));
                }
                return oState;
            }

            static void ReadBlendSpace(AnimationStateDescriptor& oState, simdjson::dom::element oElement, const std::string& sPath)
            {
                const simdjson::dom::object oObject = Json::RequireObject(oElement, sPath, {"parameter", "entries"});
                oState.sBlendParameterName = Json::RequireNameField(oObject, "parameter", sPath);

                const std::string sEntriesPath = Json::JoinPath(sPath, "entries");
                size_t uIndex = 0;
                const simdjson::dom::array oEntries = Json::RequireArray(Json::RequireField(oObject, "entries", sPath), sEntriesPath);
                for (const simdjson::dom::element oEntryElement : oEntries)
                {
                    const std::string sEntryPath = Json::IndexPath(sEntriesPath, uIndex++);
                    const simdjson::dom::object oEntry = Json::RequireObject(oEntryElement, sEntryPath, {"threshold", "clip"});
                    BlendSpaceEntryDescriptor oDescriptor;
                    oDescriptor.fThreshold = Json::RequireNumberField(oEntry, "threshold", sEntryPath);
                    oDescriptor.sClipName = Json::RequireNameField(oEntry, "clip", sEntryPath);
                    oState.vBlendEntries.push_back(std::move(oDescriptor));
                }
            }

            static AnimationTransitionDescriptor ReadTransition(simdjson::dom::element oElement, const std::string& sPath)
            {
                const simdjson::dom::object oObject =
                    Json::RequireObject(oElement, sPath, {"from", "to", "duration", "exitTime", "conditions"});

                AnimationTransitionDescriptor oTransition;
                oTransition.sFromState = Json::RequireNameField(oObject, "from", sPath);
                oTransition.sToState = Json::RequireNameField(oObject, "to", sPath);
                if (const std::optional<simdjson::dom::element> optDuration = Json::FindField(oObject, "duration"))
                {
                    oTransition.fDurationSeconds = Json::RequireNumber(*optDuration, Json::JoinPath(sPath, "duration"));
                }
                if (const std::optional<simdjson::dom::element> optExitTime = Json::FindField(oObject, "exitTime"))
                {
                    oTransition.optExitTime = Json::RequireNumber(*optExitTime, Json::JoinPath(sPath, "exitTime"));
                }
                if (const std::optional<simdjson::dom::element> optConditions = Json::FindField(oObject, "conditions"))
                {
                    const std::string sConditionsPath = Json::JoinPath(sPath, "conditions");
                    size_t uIndex = 0;
                    for (const simdjson::dom::element oCondition : Json::RequireArray(*optConditions, sConditionsPath))
                    {
                        oTransition.vConditions.push_back(ReadCondition(oCondition, Json::IndexPath(sConditionsPath, uIndex++)));
                    }
                }
                return oTransition;
            }

            static AnimationConditionDescriptor ReadCondition(simdjson::dom::element oElement, const std::string& sPath)
            {
                const simdjson::dom::object oObject =
                    Json::RequireObject(oElement, sPath, {"parameter", "greater", "less", "equals", "triggered"});

                AnimationConditionDescriptor oCondition;
                oCondition.sParameterName = Json::RequireNameField(oObject, "parameter", sPath);

                const std::optional<simdjson::dom::element> optGreater = Json::FindField(oObject, "greater");
                const std::optional<simdjson::dom::element> optLess = Json::FindField(oObject, "less");
                const std::optional<simdjson::dom::element> optEquals = Json::FindField(oObject, "equals");
                const std::optional<simdjson::dom::element> optTriggered = Json::FindField(oObject, "triggered");
                const int iOperatorCount = static_cast<int>(optGreater.has_value()) + static_cast<int>(optLess.has_value()) +
                                           static_cast<int>(optEquals.has_value()) + static_cast<int>(optTriggered.has_value());
                if (iOperatorCount != 1)
                {
                    Json::Fail(sPath, "a condition must have exactly one of 'greater', 'less', 'equals' or 'triggered'");
                }

                if (optGreater)
                {
                    oCondition.eOperator = AnimationConditionOperator::Greater;
                    oCondition.fThreshold = Json::RequireNumber(*optGreater, Json::JoinPath(sPath, "greater"));
                }
                else if (optLess)
                {
                    oCondition.eOperator = AnimationConditionOperator::Less;
                    oCondition.fThreshold = Json::RequireNumber(*optLess, Json::JoinPath(sPath, "less"));
                }
                else if (optEquals)
                {
                    const bool bExpected = Json::RequireBool(*optEquals, Json::JoinPath(sPath, "equals"));
                    oCondition.eOperator = bExpected ? AnimationConditionOperator::IsTrue : AnimationConditionOperator::IsFalse;
                }
                else
                {
                    const std::string sTriggeredPath = Json::JoinPath(sPath, "triggered");
                    if (!Json::RequireBool(*optTriggered, sTriggeredPath))
                    {
                        Json::Fail(sTriggeredPath, "must be true");
                    }
                    oCondition.eOperator = AnimationConditionOperator::Triggered;
                }
                return oCondition;
            }
        };
    }

    AnimationControllerDescriptor AnimationControllerDescriptor::FromJson(std::string_view svJson)
    {
        simdjson::dom::parser oParser;
        const simdjson::padded_string oPadded(svJson);
        simdjson::dom::element oRoot;
        const simdjson::error_code eError = oParser.parse(oPadded).get(oRoot);
        if (eError != simdjson::SUCCESS)
        {
            AnimationJsonReader::Fail("document", simdjson::error_message(eError));
        }
        return ControllerJsonParser::Parse(oRoot);
    }
}
