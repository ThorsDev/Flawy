#pragma once

#include <simdjson.h>

#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>

namespace Flawy::Animation
{
    class AnimationJsonReader final
    {
    public:
        AnimationJsonReader() = delete;

        [[noreturn]] static void Fail(std::string_view svPath, std::string_view svReason);
        [[nodiscard]] static std::string JoinPath(std::string_view svParent, std::string_view svKey);
        [[nodiscard]] static std::string IndexPath(std::string_view svParent, size_t uIndex);

        [[nodiscard]] static simdjson::dom::object RequireObject(simdjson::dom::element oElement,
                                                                 std::string_view svPath,
                                                                 std::initializer_list<std::string_view> oAllowedKeys);
        [[nodiscard]] static simdjson::dom::array RequireArray(simdjson::dom::element oElement, std::string_view svPath);
        [[nodiscard]] static std::string_view RequireString(simdjson::dom::element oElement, std::string_view svPath);
        [[nodiscard]] static std::string RequireName(simdjson::dom::element oElement, std::string_view svPath);
        [[nodiscard]] static int64_t RequireInteger(simdjson::dom::element oElement, std::string_view svPath);
        [[nodiscard]] static float RequireNumber(simdjson::dom::element oElement, std::string_view svPath);
        [[nodiscard]] static bool RequireBool(simdjson::dom::element oElement, std::string_view svPath);
        [[nodiscard]] static simdjson::dom::element RequireField(simdjson::dom::object oObject,
                                                                 std::string_view svKey,
                                                                 std::string_view svPath);
        [[nodiscard]] static std::string RequireNameField(simdjson::dom::object oObject, std::string_view svKey, std::string_view svPath);
        [[nodiscard]] static float RequireNumberField(simdjson::dom::object oObject, std::string_view svKey, std::string_view svPath);
        [[nodiscard]] static std::optional<simdjson::dom::element> FindField(simdjson::dom::object oObject, std::string_view svKey);
    };
}
