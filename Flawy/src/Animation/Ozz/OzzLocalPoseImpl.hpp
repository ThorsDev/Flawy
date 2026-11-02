#pragma once

#include "Flawy/Animation/LocalPose.hpp"

#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/soa_transform.h>

namespace Flawy::Animation
{
    struct LocalPose::Impl
    {
        const Skeleton* pSkeleton = nullptr;
        ozz::vector<ozz::math::SoaTransform> vTransforms;
    };
}
