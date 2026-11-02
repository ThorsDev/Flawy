#include "Animation/AnimationControllerValidator.hpp"

#include "Animation/AnimationParameterTypeName.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <stdexcept>
#include <string>
#include <unordered_set>

namespace Flawy::Animation
{
    namespace
    {
        std::string_view GetOperatorName(AnimationConditionOperator eOperator) noexcept
        {
            switch (eOperator)
            {
            case AnimationConditionOperator::Greater:
                return "greater";
            case AnimationConditionOperator::Less:
                return "less";
            case AnimationConditionOperator::IsTrue:
                return "is-true";
            case AnimationConditionOperator::IsFalse:
                return "is-false";
            case AnimationConditionOperator::Triggered:
                return "triggered";
            }
            return "unknown";
        }

        AnimationParameterType GetRequiredType(AnimationConditionOperator eOperator) noexcept
        {
            switch (eOperator)
            {
            case AnimationConditionOperator::Greater:
            case AnimationConditionOperator::Less:
                return AnimationParameterType::Float;
            case AnimationConditionOperator::IsTrue:
            case AnimationConditionOperator::IsFalse:
                return AnimationParameterType::Bool;
            case AnimationConditionOperator::Triggered:
                return AnimationParameterType::Trigger;
            }
            return AnimationParameterType::Trigger;
        }
    }

    AnimationControllerValidator::AnimationControllerValidator(const AnimationControllerDescriptor& oDescriptor) noexcept
        : m_oDescriptor(oDescriptor)
    {
    }

    void AnimationControllerValidator::Validate() const
    {
        if (m_oDescriptor.sName.empty())
        {
            Fail("must have a name");
        }
        ValidateParameters();
        ValidateStates();
        if (!HasState(m_oDescriptor.sEntryState))
        {
            Fail(std::format("entry state '{}' does not exist", m_oDescriptor.sEntryState));
        }
        ValidateTransitions();
    }

    void AnimationControllerValidator::ValidateParameters() const
    {
        std::unordered_set<std::string_view> setNames;
        for (const AnimationParameterDescriptor& oParameter : m_oDescriptor.vParameters)
        {
            ValidateParameter(oParameter);
            if (!setNames.insert(oParameter.sName).second)
            {
                Fail(std::format("parameter '{}' is declared more than once", oParameter.sName));
            }
        }
    }

    void AnimationControllerValidator::ValidateParameter(const AnimationParameterDescriptor& oParameter) const
    {
        if (oParameter.sName.empty())
        {
            Fail("has a parameter with an empty name");
        }
        if (!std::isfinite(oParameter.fDefaultValue))
        {
            Fail(std::format("parameter '{}' has a non-finite default value", oParameter.sName));
        }
        const bool bHasFloatDefault = oParameter.fDefaultValue != 0.0f;
        const bool bHasBoolDefault = oParameter.bDefaultValue;
        if (oParameter.eType == AnimationParameterType::Trigger && (bHasFloatDefault || bHasBoolDefault))
        {
            Fail(std::format("trigger parameter '{}' must not have a default value", oParameter.sName));
        }
        if (oParameter.eType == AnimationParameterType::Float && bHasBoolDefault)
        {
            Fail(std::format("float parameter '{}' must not have a boolean default value", oParameter.sName));
        }
        if (oParameter.eType == AnimationParameterType::Bool && bHasFloatDefault)
        {
            Fail(std::format("bool parameter '{}' must not have a numeric default value", oParameter.sName));
        }
    }

    void AnimationControllerValidator::ValidateStates() const
    {
        if (m_oDescriptor.vStates.empty())
        {
            Fail("must contain at least one state");
        }

        std::unordered_set<std::string_view> setNames;
        for (const AnimationStateDescriptor& oState : m_oDescriptor.vStates)
        {
            ValidateState(oState);
            if (!setNames.insert(oState.sName).second)
            {
                Fail(std::format("state '{}' is declared more than once", oState.sName));
            }
        }
    }

    void AnimationControllerValidator::ValidateState(const AnimationStateDescriptor& oState) const
    {
        if (oState.sName.empty())
        {
            Fail("has a state with an empty name");
        }
        if (oState.sName == AnimationTransitionDescriptor::kAnyState)
        {
            Fail(std::format("state name '{}' is reserved for any-state transitions", AnimationTransitionDescriptor::kAnyState));
        }
        if (!std::isfinite(oState.fSpeed) || oState.fSpeed <= 0.0f)
        {
            Fail(std::format("state '{}' has speed {}; the speed must be finite and greater than zero", oState.sName, oState.fSpeed));
        }

        const bool bHasClip = !oState.sClipName.empty();
        const bool bHasBlend = !oState.sBlendParameterName.empty() || !oState.vBlendEntries.empty();
        if (bHasClip == bHasBlend)
        {
            Fail(std::format("state '{}' must have exactly one of a clip or a blend space", oState.sName));
        }
        if (bHasBlend)
        {
            ValidateBlendSpace(oState);
        }
    }

    void AnimationControllerValidator::ValidateBlendSpace(const AnimationStateDescriptor& oState) const
    {
        const AnimationParameterDescriptor* pParameter = FindParameter(oState.sBlendParameterName);
        if (pParameter == nullptr)
        {
            Fail(std::format("state '{}' blends by unknown parameter '{}'", oState.sName, oState.sBlendParameterName));
        }
        if (pParameter->eType != AnimationParameterType::Float)
        {
            Fail(std::format("state '{}' blends by {} parameter '{}'; blend parameters must be float",
                             oState.sName,
                             AnimationParameterTypeName::Get(pParameter->eType),
                             pParameter->sName));
        }
        if (oState.vBlendEntries.empty())
        {
            Fail(std::format("state '{}' has a blend space without entries", oState.sName));
        }

        for (size_t uEntry = 0; uEntry < oState.vBlendEntries.size(); ++uEntry)
        {
            const BlendSpaceEntryDescriptor& oEntry = oState.vBlendEntries[uEntry];
            if (oEntry.sClipName.empty())
            {
                Fail(std::format("state '{}' blend entry {} has no clip", oState.sName, uEntry));
            }
            if (!std::isfinite(oEntry.fThreshold))
            {
                Fail(std::format("state '{}' blend entry {} has a non-finite threshold", oState.sName, uEntry));
            }
            if (uEntry > 0 && oEntry.fThreshold <= oState.vBlendEntries[uEntry - 1].fThreshold)
            {
                Fail(std::format("state '{}' blend entry {} threshold {} is not strictly greater than the previous one",
                                 oState.sName,
                                 uEntry,
                                 oEntry.fThreshold));
            }
        }
    }

    void AnimationControllerValidator::ValidateTransitions() const
    {
        for (size_t uTransition = 0; uTransition < m_oDescriptor.vTransitions.size(); ++uTransition)
        {
            ValidateTransition(m_oDescriptor.vTransitions[uTransition], uTransition);
        }
    }

    void AnimationControllerValidator::ValidateTransition(const AnimationTransitionDescriptor& oTransition, size_t uIndex) const
    {
        const std::string sLabel = std::format("transition {} ('{}' -> '{}')", uIndex, oTransition.sFromState, oTransition.sToState);
        const bool bFromAny = oTransition.sFromState == AnimationTransitionDescriptor::kAnyState;
        if (!bFromAny && !HasState(oTransition.sFromState))
        {
            Fail(std::format("{} starts from unknown state '{}'", sLabel, oTransition.sFromState));
        }
        if (!HasState(oTransition.sToState))
        {
            Fail(std::format("{} targets unknown state '{}'", sLabel, oTransition.sToState));
        }
        if (oTransition.sFromState == oTransition.sToState)
        {
            Fail(std::format("{} must not target its own source state", sLabel));
        }
        if (!std::isfinite(oTransition.fDurationSeconds) || oTransition.fDurationSeconds < 0.0f)
        {
            Fail(std::format("{} has duration {}; the duration must be finite and non-negative", sLabel, oTransition.fDurationSeconds));
        }
        if (oTransition.optExitTime)
        {
            const float fExitTime = *oTransition.optExitTime;
            if (!std::isfinite(fExitTime) || fExitTime < 0.0f || fExitTime > 1.0f)
            {
                Fail(std::format("{} has exit time {} outside [0, 1]", sLabel, fExitTime));
            }
            if (bFromAny)
            {
                Fail(std::format("{} is an any-state transition and must not have an exit time", sLabel));
            }
        }
        if (oTransition.vConditions.empty() && !oTransition.optExitTime)
        {
            Fail(std::format("{} needs at least one condition or an exit time", sLabel));
        }

        for (size_t uCondition = 0; uCondition < oTransition.vConditions.size(); ++uCondition)
        {
            ValidateCondition(oTransition.vConditions[uCondition], uIndex, uCondition);
        }
    }

    void AnimationControllerValidator::ValidateCondition(const AnimationConditionDescriptor& oCondition,
                                                         size_t uTransition,
                                                         size_t uCondition) const
    {
        const std::string sLabel = std::format("transition {} condition {}", uTransition, uCondition);
        const AnimationParameterDescriptor* pParameter = FindParameter(oCondition.sParameterName);
        if (pParameter == nullptr)
        {
            Fail(std::format("{} references unknown parameter '{}'", sLabel, oCondition.sParameterName));
        }

        const AnimationParameterType eRequired = GetRequiredType(oCondition.eOperator);
        if (pParameter->eType != eRequired)
        {
            Fail(std::format("{} uses operator '{}' on {} parameter '{}'; the operator requires a {} parameter",
                             sLabel,
                             GetOperatorName(oCondition.eOperator),
                             AnimationParameterTypeName::Get(pParameter->eType),
                             pParameter->sName,
                             AnimationParameterTypeName::Get(eRequired)));
        }
        if (!std::isfinite(oCondition.fThreshold))
        {
            Fail(std::format("{} has a non-finite threshold", sLabel));
        }
    }

    const AnimationParameterDescriptor* AnimationControllerValidator::FindParameter(std::string_view svName) const noexcept
    {
        const auto itParameter = std::ranges::find(m_oDescriptor.vParameters, svName, &AnimationParameterDescriptor::sName);
        return itParameter == m_oDescriptor.vParameters.end() ? nullptr : &*itParameter;
    }

    bool AnimationControllerValidator::HasState(std::string_view svName) const noexcept
    {
        return std::ranges::find(m_oDescriptor.vStates, svName, &AnimationStateDescriptor::sName) != m_oDescriptor.vStates.end();
    }

    void AnimationControllerValidator::Fail(std::string_view svReason) const
    {
        throw std::invalid_argument(std::format("Animation controller '{}': {}", m_oDescriptor.sName, svReason));
    }
}
