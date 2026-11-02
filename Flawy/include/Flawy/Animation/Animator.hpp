#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace Flawy::Animation
{
    class AnimationController;
    class IAnimationClipProvider;
    class ModelPose;
    class Skeleton;

    class Animator final
    {
    public:
        Animator(const AnimationController& oController, const Skeleton& oSkeleton, const IAnimationClipProvider& oClips);
        ~Animator();

        Animator(const Animator&) = delete;
        Animator& operator=(const Animator&) = delete;
        Animator(Animator&&) = delete;
        Animator& operator=(Animator&&) = delete;

        void SetFloat(std::string_view svName, float fValue);
        void SetBool(std::string_view svName, bool bValue);
        void SetTrigger(std::string_view svName);
        void ResetTrigger(std::string_view svName);
        [[nodiscard]] float GetFloat(std::string_view svName) const;
        [[nodiscard]] bool GetBool(std::string_view svName) const;

        void Update(float fDeltaSeconds);

        [[nodiscard]] const AnimationController& GetController() const noexcept;
        [[nodiscard]] uint32_t GetCurrentStateIndex() const noexcept;
        [[nodiscard]] const std::string& GetCurrentStateName() const noexcept;
        [[nodiscard]] float GetCurrentStatePhase() const noexcept;
        [[nodiscard]] bool IsInTransition() const noexcept;
        [[nodiscard]] std::optional<uint32_t> GetTargetStateIndex() const noexcept;
        [[nodiscard]] const ModelPose& GetModelPose() const noexcept;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_upImpl;
    };
}
