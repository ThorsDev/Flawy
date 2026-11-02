#include "Flawy/Animation/AnimationController.hpp"

#include "Animation/AnimationControllerValidator.hpp"

#include <format>
#include <stdexcept>
#include <utility>

namespace Flawy::Animation
{
    AnimationController::AnimationController(const AnimationControllerDescriptor& oDescriptor)
    {
        AnimationControllerValidator(oDescriptor).Validate();
        m_sName = oDescriptor.sName;
        ResolveParameters(oDescriptor);
        ResolveStates(oDescriptor);
        ResolveTransitions(oDescriptor);
        m_uEntryState = m_mapStates.at(oDescriptor.sEntryState);
    }

    const std::string& AnimationController::GetName() const noexcept
    {
        return m_sName;
    }

    uint32_t AnimationController::GetParameterCount() const noexcept
    {
        return static_cast<uint32_t>(m_vParameters.size());
    }

    const AnimationParameter& AnimationController::GetParameter(uint32_t uParameter) const
    {
        if (uParameter >= m_vParameters.size())
        {
            throw std::out_of_range(std::format("Animation controller '{}' has no parameter {}", m_sName, uParameter));
        }
        return m_vParameters[uParameter];
    }

    std::optional<uint32_t> AnimationController::FindParameter(std::string_view svName) const noexcept
    {
        return FindIndex(m_mapParameters, svName);
    }

    uint32_t AnimationController::GetStateCount() const noexcept
    {
        return static_cast<uint32_t>(m_vStates.size());
    }

    const AnimationState& AnimationController::GetState(uint32_t uState) const
    {
        if (uState >= m_vStates.size())
        {
            throw std::out_of_range(std::format("Animation controller '{}' has no state {}", m_sName, uState));
        }
        return m_vStates[uState];
    }

    std::optional<uint32_t> AnimationController::FindState(std::string_view svName) const noexcept
    {
        return FindIndex(m_mapStates, svName);
    }

    uint32_t AnimationController::GetEntryStateIndex() const noexcept
    {
        return m_uEntryState;
    }

    std::span<const AnimationTransition> AnimationController::GetTransitions() const noexcept
    {
        return m_vTransitions;
    }

    void AnimationController::ResolveParameters(const AnimationControllerDescriptor& oDescriptor)
    {
        m_vParameters.reserve(oDescriptor.vParameters.size());
        for (const AnimationParameterDescriptor& oSource : oDescriptor.vParameters)
        {
            m_mapParameters.emplace(oSource.sName, static_cast<uint32_t>(m_vParameters.size()));
            m_vParameters.push_back({oSource.sName, oSource.eType, oSource.fDefaultValue, oSource.bDefaultValue});
        }
    }

    void AnimationController::ResolveStates(const AnimationControllerDescriptor& oDescriptor)
    {
        m_vStates.reserve(oDescriptor.vStates.size());
        for (const AnimationStateDescriptor& oSource : oDescriptor.vStates)
        {
            AnimationState oState;
            oState.sName = oSource.sName;
            oState.fSpeed = oSource.fSpeed;
            oState.bLoop = oSource.bLoop;
            if (oSource.sClipName.empty())
            {
                oState.optBlendParameter = m_mapParameters.at(oSource.sBlendParameterName);
                for (const BlendSpaceEntryDescriptor& oEntry : oSource.vBlendEntries)
                {
                    oState.vEntries.push_back({oEntry.fThreshold, oEntry.sClipName});
                }
            }
            else
            {
                oState.vEntries.push_back({0.0f, oSource.sClipName});
            }

            m_mapStates.emplace(oSource.sName, static_cast<uint32_t>(m_vStates.size()));
            m_vStates.push_back(std::move(oState));
        }
    }

    void AnimationController::ResolveTransitions(const AnimationControllerDescriptor& oDescriptor)
    {
        m_vTransitions.reserve(oDescriptor.vTransitions.size());
        for (const AnimationTransitionDescriptor& oSource : oDescriptor.vTransitions)
        {
            AnimationTransition oTransition;
            if (oSource.sFromState != AnimationTransitionDescriptor::kAnyState)
            {
                oTransition.optFromState = m_mapStates.at(oSource.sFromState);
            }
            oTransition.uToState = m_mapStates.at(oSource.sToState);
            oTransition.fDurationSeconds = oSource.fDurationSeconds;
            oTransition.optExitTime = oSource.optExitTime;
            for (const AnimationConditionDescriptor& oCondition : oSource.vConditions)
            {
                const uint32_t uParameter = m_mapParameters.at(oCondition.sParameterName);
                oTransition.vConditions.push_back({uParameter, oCondition.eOperator, oCondition.fThreshold});
            }
            m_vTransitions.push_back(std::move(oTransition));
        }
    }

    std::optional<uint32_t> AnimationController::FindIndex(const IndexMap& mapIndices, std::string_view svName) noexcept
    {
        const auto itEntry = mapIndices.find(svName);
        if (itEntry == mapIndices.end())
        {
            return std::nullopt;
        }
        return itEntry->second;
    }
}
