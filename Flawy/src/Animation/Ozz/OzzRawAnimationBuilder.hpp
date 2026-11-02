#pragma once

#include "Flawy/Animation/AnimationClipDescriptor.hpp"

#include <ozz/animation/offline/raw_animation.h>

#include <cstdint>
#include <vector>

namespace Flawy::Animation
{
    class Skeleton;

    class OzzRawAnimationBuilder final
    {
    public:
        OzzRawAnimationBuilder(const Skeleton& oSkeleton, const AnimationClipDescriptor& oDescriptor);

        [[nodiscard]] ozz::animation::offline::RawAnimation Build() const;

    private:
        void FillTrack(ozz::animation::offline::RawAnimation::JointTrack& oTrack, uint32_t uJoint) const;

        const Skeleton& m_oSkeleton;
        const AnimationClipDescriptor& m_oDescriptor;
        std::vector<const JointTrackDescriptor*> m_vTracksByJoint;
    };
}
