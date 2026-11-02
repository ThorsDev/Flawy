#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Flawy::Animation
{
    enum class AnimationParameterType : uint8_t
    {
        Float,
        Bool,
        Trigger
    };

    enum class AnimationConditionOperator : uint8_t
    {
        Greater,
        Less,
        IsTrue,
        IsFalse,
        Triggered
    };

    struct AnimationParameterDescriptor
    {
        std::string sName;
        AnimationParameterType eType = AnimationParameterType::Float;
        float fDefaultValue = 0.0f;
        bool bDefaultValue = false;
    };

    struct BlendSpaceEntryDescriptor
    {
        float fThreshold = 0.0f;
        std::string sClipName;
    };

    struct AnimationStateDescriptor
    {
        std::string sName;
        std::string sClipName;
        std::string sBlendParameterName;
        std::vector<BlendSpaceEntryDescriptor> vBlendEntries;
        float fSpeed = 1.0f;
        bool bLoop = true;
    };

    struct AnimationConditionDescriptor
    {
        std::string sParameterName;
        AnimationConditionOperator eOperator = AnimationConditionOperator::Triggered;
        float fThreshold = 0.0f;
    };

    struct AnimationTransitionDescriptor
    {
        static constexpr std::string_view kAnyState = "*";
        static constexpr float kDefaultDurationSeconds = 0.2f;

        std::string sFromState;
        std::string sToState;
        float fDurationSeconds = kDefaultDurationSeconds;
        std::optional<float> optExitTime;
        std::vector<AnimationConditionDescriptor> vConditions;
    };

    struct AnimationControllerDescriptor
    {
        static constexpr int64_t kFormatVersion = 1;

        std::string sName;
        std::string sEntryState;
        std::vector<AnimationParameterDescriptor> vParameters;
        std::vector<AnimationStateDescriptor> vStates;
        std::vector<AnimationTransitionDescriptor> vTransitions;

        [[nodiscard]] static AnimationControllerDescriptor FromJson(std::string_view svJson);
    };
}
