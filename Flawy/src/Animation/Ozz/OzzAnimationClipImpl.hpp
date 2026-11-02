#pragma once

#include "Flawy/Animation/AnimationClip.hpp"

#include <ozz/animation/runtime/animation.h>
#include <ozz/base/memory/unique_ptr.h>

#include <string>

namespace Flawy::Animation
{
    struct AnimationClip::Impl
    {
        const Skeleton* pSkeleton = nullptr;
        std::string sName;
        float fDurationSeconds = 0.0f;
        ozz::unique_ptr<ozz::animation::Animation> upAnimation;
    };
}
