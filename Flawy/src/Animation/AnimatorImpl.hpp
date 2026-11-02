#pragma once

#include "Flawy/Animation/AnimationController.hpp"
#include "Flawy/Animation/AnimationSampler.hpp"
#include "Flawy/Animation/Animator.hpp"
#include "Flawy/Animation/LocalPose.hpp"
#include "Flawy/Animation/ModelPose.hpp"
#include "Flawy/Animation/PoseBlender.hpp"

#include <array>
#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

namespace Flawy::Animation
{
    class AnimationClip;

    struct Animator::Impl
    {
        static constexpr uint32_t kMaxBlendLayers = 2;

        struct EntryRuntime
        {
            const AnimationClip* pClip = nullptr;
            std::unique_ptr<AnimationSampler> upSampler;
            float fThreshold = 0.0f;
        };

        struct StateRuntime
        {
            uint32_t uFirstEntry = 0;
            uint32_t uEntryCount = 0;
        };

        struct StatePlayback
        {
            uint32_t uState = 0;
            float fPhase = 0.0f;
            float fPreviousPhase = 0.0f;
            bool bWrapped = false;
            bool bCompletedCycle = false;
        };

        struct BlendPair
        {
            uint32_t uFirst = 0;
            uint32_t uSecond = 0;
            float fFactor = 0.0f;
        };

        struct ParameterValue
        {
            float fValue = 0.0f;
            bool bValue = false;
        };

        Impl(const AnimationController& oOwnerController, const Skeleton& oOwnerSkeleton, const IAnimationClipProvider& oClips);

        void Update(float fDeltaSeconds);
        [[nodiscard]] uint32_t RequireParameter(std::string_view svName, AnimationParameterType eType) const;

        void BuildRuntime(const IAnimationClipProvider& oClips);
        void Advance(StatePlayback& oPlayback, float fDeltaSeconds) const;
        void EvaluateTransitions();
        [[nodiscard]] bool CanFire(const AnimationTransition& oTransition) const noexcept;
        [[nodiscard]] bool HasReachedExitTime(float fExitTime) const noexcept;
        [[nodiscard]] bool IsConditionMet(uint32_t uParameter, AnimationConditionOperator eOperator, float fThreshold) const noexcept;
        void StartTransition(uint32_t uTransition);
        void CompleteTransition() noexcept;
        void EvaluatePose();
        void EvaluateState(const StatePlayback& oPlayback, LocalPose& oPose);
        [[nodiscard]] BlendPair ResolveBlendPair(uint32_t uState) const noexcept;
        [[nodiscard]] float GetStateDuration(uint32_t uState) const noexcept;

        const AnimationController& oController;
        const Skeleton& oSkeleton;
        std::vector<EntryRuntime> vEntries;
        std::vector<StateRuntime> vStates;
        std::vector<ParameterValue> vParameters;
        StatePlayback oCurrent;
        StatePlayback oTarget;
        bool bTransitioning = false;
        uint32_t uActiveTransition = 0;
        float fTransitionElapsed = 0.0f;
        LocalPose oCurrentPose;
        LocalPose oTargetPose;
        LocalPose oFirstScratchPose;
        LocalPose oSecondScratchPose;
        LocalPose oBlendedPose;
        PoseBlender oBlender;
        ModelPose oModelPose;
        std::array<BlendLayer, kMaxBlendLayers> aLayers{};
    };
}
