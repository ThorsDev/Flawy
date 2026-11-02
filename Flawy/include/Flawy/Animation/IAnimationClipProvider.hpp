#pragma once

#include <string_view>

namespace Flawy::Animation
{
    class AnimationClip;

    class IAnimationClipProvider
    {
    public:
        virtual ~IAnimationClipProvider() = default;

        [[nodiscard]] virtual const AnimationClip* FindClip(std::string_view svName) const noexcept = 0;
    };
}
