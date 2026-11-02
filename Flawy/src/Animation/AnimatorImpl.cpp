#include "Animation/AnimatorImpl.hpp"

#include "Animation/AnimationParameterTypeName.hpp"

#include "Flawy/Animation/AnimationClip.hpp"
#include "Flawy/Animation/IAnimationClipProvider.hpp"
#include "Flawy/Animation/Skeleton.hpp"
#include "Flawy/Core/Logger.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <optional>
#include <stdexcept>

namespace Flawy::Animation
{
    Animator::Impl::Impl(const AnimationController& oOwnerController, const Skeleton& oOwnerSkeleton, const IAnimationClipProvider& oClips)
        : oController(oOwnerController)
        , oSkeleton(oOwnerSkeleton)
        , oCurrentPose(oOwnerSkeleton)
        , oTargetPose(oOwnerSkeleton)
        , oFirstScratchPose(oOwnerSkeleton)
        , oSecondScratchPose(oOwnerSkeleton)
        , oBlendedPose(oOwnerSkeleton)
        , oBlender(oOwnerSkeleton, kMaxBlendLayers)
        , oModelPose(oOwnerSkeleton)
    {
        BuildRuntime(oClips);

        vParameters.reserve(oController.GetParameterCount());
        for (uint32_t uParameter = 0; uParameter < oController.GetParameterCount(); ++uParameter)
        {
            const AnimationParameter& oParameter = oController.GetParameter(uParameter);
            vParameters.push_back({oParameter.fDefaultValue, oParameter.bDefaultValue});
        }

        oCurrent.uState = oController.GetEntryStateIndex();
        EvaluatePose();
    }

    void Animator::Impl::BuildRuntime(const IAnimationClipProvider& oClips)
    {
        vStates.reserve(oController.GetStateCount());
        for (uint32_t uState = 0; uState < oController.GetStateCount(); ++uState)
        {
            const AnimationState& oState = oController.GetState(uState);
            StateRuntime oRuntime;
            oRuntime.uFirstEntry = static_cast<uint32_t>(vEntries.size());
            oRuntime.uEntryCount = static_cast<uint32_t>(oState.vEntries.size());
            for (const AnimationBlendSpaceEntry& oEntry : oState.vEntries)
            {
                const AnimationClip* pClip = oClips.FindClip(oEntry.sClipName);
                if (pClip == nullptr)
                {
                    throw std::invalid_argument(
                        std::format("Animation controller '{}': state '{}' references unknown clip '{}'",
                                    oController.GetName(),
                                    oState.sName,
                                    oEntry.sClipName));
                }
                if (&pClip->GetSkeleton() != &oSkeleton)
                {
                    throw std::invalid_argument(std::format("Animation controller '{}': state '{}' clip '{}' is bound to skeleton '{}' "
                                                            "but the animator uses skeleton '{}'",
                                                            oController.GetName(),
                                                            oState.sName,
                                                            oEntry.sClipName,
                                                            pClip->GetSkeleton().GetName(),
                                                            oSkeleton.GetName()));
                }

                EntryRuntime oEntryRuntime;
                oEntryRuntime.pClip = pClip;
                oEntryRuntime.upSampler = std::make_unique<AnimationSampler>(*pClip);
                oEntryRuntime.fThreshold = oEntry.fThreshold;
                vEntries.push_back(std::move(oEntryRuntime));
            }
            vStates.push_back(oRuntime);
        }
    }

    void Animator::Impl::Update(float fDeltaSeconds)
    {
        if (!std::isfinite(fDeltaSeconds))
        {
            throw std::invalid_argument(std::format("Animator '{}' received a non-finite delta time", oController.GetName()));
        }
        const float fDelta = std::max(fDeltaSeconds, 0.0f);

        Advance(oCurrent, fDelta);
        if (bTransitioning)
        {
            Advance(oTarget, fDelta);
            fTransitionElapsed += fDelta;
            if (fTransitionElapsed >= oController.GetTransitions()[uActiveTransition].fDurationSeconds)
            {
                CompleteTransition();
            }
        }
        else
        {
            EvaluateTransitions();
        }
        EvaluatePose();
    }

    uint32_t Animator::Impl::RequireParameter(std::string_view svName, AnimationParameterType eType) const
    {
        const std::optional<uint32_t> optParameter = oController.FindParameter(svName);
        if (!optParameter)
        {
            throw std::invalid_argument(std::format("Animation controller '{}' has no parameter '{}'", oController.GetName(), svName));
        }
        const AnimationParameter& oParameter = oController.GetParameter(*optParameter);
        if (oParameter.eType != eType)
        {
            throw std::invalid_argument(std::format("Animation controller '{}': parameter '{}' is a {} parameter, not a {} parameter",
                                                    oController.GetName(),
                                                    svName,
                                                    AnimationParameterTypeName::Get(oParameter.eType),
                                                    AnimationParameterTypeName::Get(eType)));
        }
        return *optParameter;
    }

    void Animator::Impl::Advance(StatePlayback& oPlayback, float fDeltaSeconds) const
    {
        const AnimationState& oState = oController.GetState(oPlayback.uState);
        const float fNextPhase = oPlayback.fPhase + fDeltaSeconds * oState.fSpeed / GetStateDuration(oPlayback.uState);
        oPlayback.fPreviousPhase = oPlayback.fPhase;
        oPlayback.bWrapped = false;
        oPlayback.bCompletedCycle = fNextPhase - oPlayback.fPhase >= 1.0f;
        if (!oState.bLoop)
        {
            oPlayback.fPhase = std::min(fNextPhase, 1.0f);
            return;
        }
        if (fNextPhase >= 1.0f)
        {
            oPlayback.fPhase = std::fmod(fNextPhase, 1.0f);
            oPlayback.bWrapped = true;
            return;
        }
        oPlayback.fPhase = fNextPhase;
    }

    void Animator::Impl::EvaluateTransitions()
    {
        const std::span<const AnimationTransition> spanTransitions = oController.GetTransitions();
        for (uint32_t uTransition = 0; uTransition < spanTransitions.size(); ++uTransition)
        {
            const AnimationTransition& oTransition = spanTransitions[uTransition];
            if (!oTransition.optFromState && oTransition.uToState != oCurrent.uState && CanFire(oTransition))
            {
                StartTransition(uTransition);
                return;
            }
        }
        for (uint32_t uTransition = 0; uTransition < spanTransitions.size(); ++uTransition)
        {
            const AnimationTransition& oTransition = spanTransitions[uTransition];
            if (oTransition.optFromState == oCurrent.uState && CanFire(oTransition))
            {
                StartTransition(uTransition);
                return;
            }
        }
    }

    bool Animator::Impl::CanFire(const AnimationTransition& oTransition) const noexcept
    {
        if (oTransition.optExitTime && !HasReachedExitTime(*oTransition.optExitTime))
        {
            return false;
        }
        return std::ranges::all_of(oTransition.vConditions, [this](const AnimationCondition& oCondition) {
            return IsConditionMet(oCondition.uParameter, oCondition.eOperator, oCondition.fThreshold);
        });
    }

    bool Animator::Impl::HasReachedExitTime(float fExitTime) const noexcept
    {
        if (!oController.GetState(oCurrent.uState).bLoop)
        {
            return oCurrent.fPhase >= fExitTime;
        }
        if (oCurrent.bCompletedCycle)
        {
            return true;
        }
        if (oCurrent.bWrapped)
        {
            return oCurrent.fPreviousPhase < fExitTime || oCurrent.fPhase >= fExitTime;
        }
        return oCurrent.fPreviousPhase < fExitTime && oCurrent.fPhase >= fExitTime;
    }

    bool Animator::Impl::IsConditionMet(uint32_t uParameter, AnimationConditionOperator eOperator, float fThreshold) const noexcept
    {
        const ParameterValue& oValue = vParameters[uParameter];
        switch (eOperator)
        {
        case AnimationConditionOperator::Greater:
            return oValue.fValue > fThreshold;
        case AnimationConditionOperator::Less:
            return oValue.fValue < fThreshold;
        case AnimationConditionOperator::IsTrue:
        case AnimationConditionOperator::Triggered:
            return oValue.bValue;
        case AnimationConditionOperator::IsFalse:
            return !oValue.bValue;
        }
        return false;
    }

    void Animator::Impl::StartTransition(uint32_t uTransition)
    {
        const AnimationTransition& oTransition = oController.GetTransitions()[uTransition];
        for (const AnimationCondition& oCondition : oTransition.vConditions)
        {
            if (oCondition.eOperator == AnimationConditionOperator::Triggered)
            {
                vParameters[oCondition.uParameter].bValue = false;
            }
        }

        if (Core::Logger::IsInitialized())
        {
            Core::Logger::Get().debug("Animator '{}': {} -> {} (crossfade {} s)",
                                      oController.GetName(),
                                      oController.GetState(oCurrent.uState).sName,
                                      oController.GetState(oTransition.uToState).sName,
                                      oTransition.fDurationSeconds);
        }

        oTarget = StatePlayback{oTransition.uToState, 0.0f, 0.0f, false, false};
        if (oTransition.fDurationSeconds <= 0.0f)
        {
            oCurrent = oTarget;
            return;
        }
        bTransitioning = true;
        uActiveTransition = uTransition;
        fTransitionElapsed = 0.0f;
    }

    void Animator::Impl::CompleteTransition() noexcept
    {
        oCurrent = oTarget;
        bTransitioning = false;
        fTransitionElapsed = 0.0f;
    }

    void Animator::Impl::EvaluatePose()
    {
        EvaluateState(oCurrent, oCurrentPose);
        if (!bTransitioning)
        {
            oModelPose.Update(oCurrentPose);
            return;
        }

        EvaluateState(oTarget, oTargetPose);
        const float fWeight = std::min(fTransitionElapsed / oController.GetTransitions()[uActiveTransition].fDurationSeconds, 1.0f);
        aLayers[0] = BlendLayer{&oCurrentPose, 1.0f - fWeight};
        aLayers[1] = BlendLayer{&oTargetPose, fWeight};
        oBlender.Blend(aLayers, oBlendedPose);
        oModelPose.Update(oBlendedPose);
    }

    void Animator::Impl::EvaluateState(const StatePlayback& oPlayback, LocalPose& oPose)
    {
        const BlendPair oPair = ResolveBlendPair(oPlayback.uState);
        if (oPair.uFirst == oPair.uSecond || oPair.fFactor <= 0.0f)
        {
            vEntries[oPair.uFirst].upSampler->SampleAtPhase(oPlayback.fPhase, oPose);
            return;
        }
        if (oPair.fFactor >= 1.0f)
        {
            vEntries[oPair.uSecond].upSampler->SampleAtPhase(oPlayback.fPhase, oPose);
            return;
        }

        vEntries[oPair.uFirst].upSampler->SampleAtPhase(oPlayback.fPhase, oFirstScratchPose);
        vEntries[oPair.uSecond].upSampler->SampleAtPhase(oPlayback.fPhase, oSecondScratchPose);
        aLayers[0] = BlendLayer{&oFirstScratchPose, 1.0f - oPair.fFactor};
        aLayers[1] = BlendLayer{&oSecondScratchPose, oPair.fFactor};
        oBlender.Blend(aLayers, oPose);
    }

    Animator::Impl::BlendPair Animator::Impl::ResolveBlendPair(uint32_t uState) const noexcept
    {
        const StateRuntime& oRuntime = vStates[uState];
        const uint32_t uFirst = oRuntime.uFirstEntry;
        const uint32_t uLast = oRuntime.uFirstEntry + oRuntime.uEntryCount - 1;
        const std::optional<uint32_t>& optParameter = oController.GetState(uState).optBlendParameter;
        if (uFirst == uLast || !optParameter)
        {
            return {uFirst, uFirst, 0.0f};
        }

        const float fValue = vParameters[*optParameter].fValue;
        if (fValue <= vEntries[uFirst].fThreshold)
        {
            return {uFirst, uFirst, 0.0f};
        }
        if (fValue >= vEntries[uLast].fThreshold)
        {
            return {uLast, uLast, 0.0f};
        }

        for (uint32_t uEntry = uFirst; uEntry < uLast; ++uEntry)
        {
            const float fLower = vEntries[uEntry].fThreshold;
            const float fUpper = vEntries[uEntry + 1].fThreshold;
            if (fValue < fUpper)
            {
                return {uEntry, uEntry + 1, (fValue - fLower) / (fUpper - fLower)};
            }
        }
        return {uLast, uLast, 0.0f};
    }

    float Animator::Impl::GetStateDuration(uint32_t uState) const noexcept
    {
        const BlendPair oPair = ResolveBlendPair(uState);
        const float fFirst = vEntries[oPair.uFirst].pClip->GetDurationSeconds();
        const float fSecond = vEntries[oPair.uSecond].pClip->GetDurationSeconds();
        return fFirst + (fSecond - fFirst) * oPair.fFactor;
    }
}
