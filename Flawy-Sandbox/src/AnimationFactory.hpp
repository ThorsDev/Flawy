#pragma once

#include "AnimationResources.hpp"

#include <Flawy/Flawy.hpp>

#include <string_view>

namespace Sandbox
{
    class AnimationFactory final
    {
    public:
        AnimationFactory() = delete;

        static void Create(AnimationResources& oResources, Flawy::Assets::AssetManager& oAssetManager);
        static void Release(AnimationResources& oResources) noexcept;

    private:
        static void EnsureGeneratedAssets(Flawy::Assets::AssetManager& oAssetManager);
        static void SaveSkeletonIfMissing(Flawy::Assets::AssetManager& oAssetManager, std::string_view svPath);
        static void SaveClipIfMissing(Flawy::Assets::AssetManager& oAssetManager,
                                      std::string_view svPath,
                                      const Flawy::Animation::AnimationClipDescriptor& oClip);
        [[nodiscard]] static Flawy::Animation::SkeletonDescriptor BuildSkeleton();
        [[nodiscard]] static Flawy::Animation::AnimationClipDescriptor BuildIdleClip();
        [[nodiscard]] static Flawy::Animation::AnimationClipDescriptor BuildSwayClip();
        [[nodiscard]] static Flawy::Animation::AnimationClipDescriptor BuildWaveClip();
    };
}
