#include "Flawy/Animation/AnimationSampler.hpp"

#include "Animation/Ozz/OzzAnimationClipImpl.hpp"
#include "Animation/Ozz/OzzLocalPoseImpl.hpp"
#include "Flawy/Animation/AnimationClip.hpp"
#include "Flawy/Animation/LocalPose.hpp"
#include "Flawy/Animation/Skeleton.hpp"

#include <ozz/animation/runtime/sampling_job.h>
#include <ozz/base/span.h>

#include <algorithm>
#include <cmath>
#include <format>
#include <stdexcept>

namespace Flawy::Animation
{
    struct AnimationSampler::Impl
    {
        explicit Impl(const AnimationClip& oOwner)
            : oClip(oOwner)
            , oContext(oOwner.GetImpl().upAnimation->num_tracks())
        {
        }

        const AnimationClip& oClip;
        ozz::animation::SamplingJob::Context oContext;
    };

    AnimationSampler::AnimationSampler(const AnimationClip& oClip)
        : m_upImpl(std::make_unique<Impl>(oClip))
    {
    }

    AnimationSampler::~AnimationSampler() = default;

    void AnimationSampler::SampleAtPhase(float fPhase, LocalPose& oPose)
    {
        Impl& oImpl = *m_upImpl;
        if (!std::isfinite(fPhase))
        {
            throw std::invalid_argument(std::format("Animation clip '{}' cannot be sampled at a non-finite phase", oImpl.oClip.GetName()));
        }
        if (&oPose.GetSkeleton() != &oImpl.oClip.GetSkeleton())
        {
            throw std::invalid_argument(std::format("Animation clip '{}' of skeleton '{}' cannot be sampled into a pose of skeleton '{}'",
                                                    oImpl.oClip.GetName(),
                                                    oImpl.oClip.GetSkeleton().GetName(),
                                                    oPose.GetSkeleton().GetName()));
        }

        ozz::animation::SamplingJob oJob;
        oJob.animation = oImpl.oClip.GetImpl().upAnimation.get();
        oJob.context = &oImpl.oContext;
        oJob.ratio = std::clamp(fPhase, 0.0f, 1.0f);
        oJob.output = ozz::make_span(oPose.GetImpl().vTransforms);
        if (!oJob.Run())
        {
            throw std::logic_error(std::format("Sampling job failed for animation clip '{}'", oImpl.oClip.GetName()));
        }
    }

    void AnimationSampler::SampleAtTime(float fTimeSeconds, LocalPose& oPose)
    {
        if (!std::isfinite(fTimeSeconds))
        {
            throw std::invalid_argument(std::format("Animation clip '{}' cannot be sampled at a non-finite time",
                                                    m_upImpl->oClip.GetName()));
        }
        SampleAtPhase(fTimeSeconds / m_upImpl->oClip.GetDurationSeconds(), oPose);
    }

    const AnimationClip& AnimationSampler::GetClip() const noexcept
    {
        return m_upImpl->oClip;
    }
}
