#pragma once

#include <memory>
#include <string>

namespace Flawy::Animation
{
    class Skeleton;
    struct AnimationClipDescriptor;

    class AnimationClip final
    {
    public:
        struct Impl;

        AnimationClip(const Skeleton& oSkeleton, const AnimationClipDescriptor& oDescriptor);
        ~AnimationClip();

        AnimationClip(const AnimationClip&) = delete;
        AnimationClip& operator=(const AnimationClip&) = delete;
        AnimationClip(AnimationClip&&) = delete;
        AnimationClip& operator=(AnimationClip&&) = delete;

        [[nodiscard]] const std::string& GetName() const noexcept;
        [[nodiscard]] float GetDurationSeconds() const noexcept;
        [[nodiscard]] const Skeleton& GetSkeleton() const noexcept;
        [[nodiscard]] const Impl& GetImpl() const noexcept;

    private:
        std::unique_ptr<Impl> m_upImpl;
    };
}
