#include "Flawy/Animation/AnimationClip.hpp"

#include "Animation/Ozz/OzzAnimationClipImpl.hpp"
#include "Animation/Ozz/OzzRawAnimationBuilder.hpp"
#include "Flawy/Animation/AnimationClipDescriptor.hpp"
#include "Flawy/Animation/Skeleton.hpp"

#include <ozz/animation/offline/animation_builder.h>

#include <format>
#include <stdexcept>

namespace Flawy::Animation
{
    AnimationClip::AnimationClip(const Skeleton& oSkeleton, const AnimationClipDescriptor& oDescriptor)
        : m_upImpl(std::make_unique<Impl>())
    {
        oDescriptor.Validate();
        const ozz::animation::offline::RawAnimation oRaw = OzzRawAnimationBuilder(oSkeleton, oDescriptor).Build();
        if (!oRaw.Validate())
        {
            throw std::logic_error(std::format("Animation clip '{}' produced an invalid ozz raw animation", oDescriptor.sName));
        }

        ozz::animation::offline::AnimationBuilder oBuilder;
        oBuilder.iframe_interval = 0.0f;
        m_upImpl->upAnimation = oBuilder(oRaw);
        if (!m_upImpl->upAnimation)
        {
            throw std::logic_error(std::format("Animation clip '{}' could not be built", oDescriptor.sName));
        }

        m_upImpl->pSkeleton = &oSkeleton;
        m_upImpl->sName = oDescriptor.sName;
        m_upImpl->fDurationSeconds = oDescriptor.fDurationSeconds;
    }

    AnimationClip::~AnimationClip() = default;

    const std::string& AnimationClip::GetName() const noexcept
    {
        return m_upImpl->sName;
    }

    float AnimationClip::GetDurationSeconds() const noexcept
    {
        return m_upImpl->fDurationSeconds;
    }

    const Skeleton& AnimationClip::GetSkeleton() const noexcept
    {
        return *m_upImpl->pSkeleton;
    }

    const AnimationClip::Impl& AnimationClip::GetImpl() const noexcept
    {
        return *m_upImpl;
    }
}
