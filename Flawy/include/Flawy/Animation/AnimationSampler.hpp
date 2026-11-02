#pragma once

#include <memory>

namespace Flawy::Animation
{
    class AnimationClip;
    class LocalPose;

    class AnimationSampler final
    {
    public:
        explicit AnimationSampler(const AnimationClip& oClip);
        ~AnimationSampler();

        AnimationSampler(const AnimationSampler&) = delete;
        AnimationSampler& operator=(const AnimationSampler&) = delete;
        AnimationSampler(AnimationSampler&&) = delete;
        AnimationSampler& operator=(AnimationSampler&&) = delete;

        void SampleAtPhase(float fPhase, LocalPose& oPose);
        void SampleAtTime(float fTimeSeconds, LocalPose& oPose);
        [[nodiscard]] const AnimationClip& GetClip() const noexcept;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_upImpl;
    };
}
