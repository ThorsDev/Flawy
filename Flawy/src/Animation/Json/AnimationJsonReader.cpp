#include "Animation/Json/AnimationJsonReader.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <stdexcept>
#include <unordered_set>

namespace Flawy::Animation
{
    namespace
    {
        constexpr std::string_view kDocumentPath = "document";
    }

    void AnimationJsonReader::Fail(std::string_view svPath, std::string_view svReason)
    {
        throw std::invalid_argument(std::format("Animation controller JSON {}: {}", svPath, svReason));
    }

    std::string AnimationJsonReader::JoinPath(std::string_view svParent, std::string_view svKey)
    {
        return svParent == kDocumentPath ? std::string(svKey) : std::format("{}.{}", svParent, svKey);
    }

    std::string AnimationJsonReader::IndexPath(std::string_view svParent, size_t uIndex)
    {
        return std::format("{}[{}]", svParent, uIndex);
    }

    simdjson::dom::object AnimationJsonReader::RequireObject(simdjson::dom::element oElement,
                                                             std::string_view svPath,
                                                             std::initializer_list<std::string_view> oAllowedKeys)
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
                Fail(JoinPath(svPath, oField.key), "duplicate key");
            }
            if (std::ranges::find(oAllowedKeys, oField.key) == oAllowedKeys.end())
            {
                Fail(JoinPath(svPath, oField.key), "unexpected field");
            }
        }
        return oObject;
    }

    simdjson::dom::array AnimationJsonReader::RequireArray(simdjson::dom::element oElement, std::string_view svPath)
    {
        simdjson::dom::array oArray;
        if (oElement.get_array().get(oArray) != simdjson::SUCCESS)
        {
            Fail(svPath, "expected an array");
        }
        return oArray;
    }

    std::string_view AnimationJsonReader::RequireString(simdjson::dom::element oElement, std::string_view svPath)
    {
        std::string_view svValue;
        if (oElement.get_string().get(svValue) != simdjson::SUCCESS)
        {
            Fail(svPath, "expected a string");
        }
        return svValue;
    }

    std::string AnimationJsonReader::RequireName(simdjson::dom::element oElement, std::string_view svPath)
    {
        const std::string_view svValue = RequireString(oElement, svPath);
        if (svValue.empty())
        {
            Fail(svPath, "must not be empty");
        }
        return std::string(svValue);
    }

    int64_t AnimationJsonReader::RequireInteger(simdjson::dom::element oElement, std::string_view svPath)
    {
        int64_t iValue = 0;
        if (oElement.get_int64().get(iValue) != simdjson::SUCCESS)
        {
            Fail(svPath, "expected an integer");
        }
        return iValue;
    }

    float AnimationJsonReader::RequireNumber(simdjson::dom::element oElement, std::string_view svPath)
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

    bool AnimationJsonReader::RequireBool(simdjson::dom::element oElement, std::string_view svPath)
    {
        bool bValue = false;
        if (oElement.get_bool().get(bValue) != simdjson::SUCCESS)
        {
            Fail(svPath, "expected a boolean");
        }
        return bValue;
    }

    simdjson::dom::element AnimationJsonReader::RequireField(simdjson::dom::object oObject, std::string_view svKey, std::string_view svPath)
    {
        simdjson::dom::element oElement;
        if (oObject.at_key(svKey).get(oElement) != simdjson::SUCCESS)
        {
            Fail(svPath, std::format("missing field '{}'", svKey));
        }
        return oElement;
    }

    std::string AnimationJsonReader::RequireNameField(simdjson::dom::object oObject, std::string_view svKey, std::string_view svPath)
    {
        return RequireName(RequireField(oObject, svKey, svPath), JoinPath(svPath, svKey));
    }

    float AnimationJsonReader::RequireNumberField(simdjson::dom::object oObject, std::string_view svKey, std::string_view svPath)
    {
        return RequireNumber(RequireField(oObject, svKey, svPath), JoinPath(svPath, svKey));
    }

    std::optional<simdjson::dom::element> AnimationJsonReader::FindField(simdjson::dom::object oObject, std::string_view svKey)
    {
        simdjson::dom::element oElement;
        if (oObject.at_key(svKey).get(oElement) != simdjson::SUCCESS)
        {
            return std::nullopt;
        }
        return oElement;
    }
}
