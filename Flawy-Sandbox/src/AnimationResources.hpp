#pragma once

#include <Flawy/Flawy.hpp>

#include <memory>

namespace Sandbox
{
    struct AnimationResources
    {
        Flawy::Assets::AssetHandle<Flawy::Animation::SkeletonAsset> oSkeleton;
        Flawy::Assets::AssetHandle<Flawy::Animation::AnimationControllerAsset> oController;
        std::unique_ptr<Flawy::Animation::AnimationClipLibrary> upClipLibrary;
        std::unique_ptr<Flawy::Animation::Animator> upAnimator;
    };
}
