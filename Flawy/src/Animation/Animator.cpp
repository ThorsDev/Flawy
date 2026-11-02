#include "Flawy/Animation/Animator.hpp"

#include "Animation/AnimatorImpl.hpp"

#include <cmath>
#include <format>
#include <stdexcept>

namespace Flawy::Animation
{
    Animator::Animator(const AnimationController& oController, const Skeleton& oSkeleton, const IAnimationClipProvider& oClips)
        : m_upImpl(std::make_unique<Impl>(oController, oSkeleton, oClips))
    {
    }

    Animator::~Animator() = default;

    void Animator::SetFloat(std::string_view svName, float fValue)
    {
        const uint32_t uParameter = m_upImpl->RequireParameter(svName, AnimationParameterType::Float);
        if (!std::isfinite(fValue))
        {
            throw std::invalid_argument(std::format("Animation parameter '{}' must be set to a finite value", svName));
        }
        m_upImpl->vParameters[uParameter].fValue = fValue;
    }

    void Animator::SetBool(std::string_view svName, bool bValue)
    {
        m_upImpl->vParameters[m_upImpl->RequireParameter(svName, AnimationParameterType::Bool)].bValue = bValue;
    }

    void Animator::SetTrigger(std::string_view svName)
    {
        m_upImpl->vParameters[m_upImpl->RequireParameter(svName, AnimationParameterType::Trigger)].bValue = true;
    }

    void Animator::ResetTrigger(std::string_view svName)
    {
        m_upImpl->vParameters[m_upImpl->RequireParameter(svName, AnimationParameterType::Trigger)].bValue = false;
    }

    float Animator::GetFloat(std::string_view svName) const
    {
        return m_upImpl->vParameters[m_upImpl->RequireParameter(svName, AnimationParameterType::Float)].fValue;
    }

    bool Animator::GetBool(std::string_view svName) const
    {
        return m_upImpl->vParameters[m_upImpl->RequireParameter(svName, AnimationParameterType::Bool)].bValue;
    }

    void Animator::Update(float fDeltaSeconds)
    {
        m_upImpl->Update(fDeltaSeconds);
    }

    const AnimationController& Animator::GetController() const noexcept
    {
        return m_upImpl->oController;
    }

    uint32_t Animator::GetCurrentStateIndex() const noexcept
    {
        return m_upImpl->oCurrent.uState;
    }

    const std::string& Animator::GetCurrentStateName() const noexcept
    {
        return m_upImpl->oController.GetState(m_upImpl->oCurrent.uState).sName;
    }

    float Animator::GetCurrentStatePhase() const noexcept
    {
        return m_upImpl->oCurrent.fPhase;
    }

    bool Animator::IsInTransition() const noexcept
    {
        return m_upImpl->bTransitioning;
    }

    std::optional<uint32_t> Animator::GetTargetStateIndex() const noexcept
    {
        if (!m_upImpl->bTransitioning)
        {
            return std::nullopt;
        }
        return m_upImpl->oTarget.uState;
    }

    const ModelPose& Animator::GetModelPose() const noexcept
    {
        return m_upImpl->oModelPose;
    }
}
