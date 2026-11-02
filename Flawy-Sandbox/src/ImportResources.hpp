#pragma once

#include <Flawy/Flawy.hpp>

#include <glm/mat4x4.hpp>

#include <memory>
#include <vector>

namespace Sandbox
{
    struct ImportedModelInstance
    {
        Flawy::Assets::AssetHandle<Flawy::Assets::StaticMeshAsset> oMesh;
        glm::mat4 matModel{1.0f};
    };

    struct ImportResources
    {
        std::vector<ImportedModelInstance> vModels;
        Flawy::Assets::AssetHandle<Flawy::Animation::SkeletonAsset> oSkeleton;
        std::unique_ptr<Flawy::Animation::AnimationController> upController;
        std::unique_ptr<Flawy::Animation::AnimationClipLibrary> upClipLibrary;
        std::unique_ptr<Flawy::Animation::Animator> upAnimator;
        glm::mat4 matJointOrigin{1.0f};
    };
}
