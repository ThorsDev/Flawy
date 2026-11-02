#pragma once

#include "Flawy/Animation/SkeletonDescriptor.hpp"

#include <ozz/animation/offline/raw_skeleton.h>

#include <cstdint>
#include <vector>

namespace Flawy::Animation
{
    class OzzRawSkeletonBuilder final
    {
    public:
        explicit OzzRawSkeletonBuilder(const SkeletonDescriptor& oDescriptor);

        [[nodiscard]] ozz::animation::offline::RawSkeleton Build() const;

    private:
        void AppendChildren(ozz::animation::offline::RawSkeleton::Joint::Children& vTarget, int32_t iParentIndex) const;

        const SkeletonDescriptor& m_oDescriptor;
        std::vector<std::vector<uint32_t>> m_vChildren;
        std::vector<uint32_t> m_vRoots;
    };
}
