#pragma once

#include "ImportResources.hpp"

#include <Flawy/Flawy.hpp>

#include <glm/mat4x4.hpp>

#include <optional>
#include <string>
#include <string_view>

namespace Sandbox
{
    class ImportFactory final
    {
    public:
        ImportFactory() = delete;

        static void Create(ImportResources& oResources, Flawy::Assets::AssetManager& oAssetManager, const std::optional<std::string>& optImportPath);
        static void Update(ImportResources& oResources, double dDeltaSeconds);
        static void Release(ImportResources& oResources) noexcept;

    private:
        static void CreateDefaultScene(ImportResources& oResources, Flawy::Assets::AssetManager& oAssetManager);
        static void CreateCustomScene(ImportResources& oResources, Flawy::Assets::AssetManager& oAssetManager, std::string_view svSourcePath);
        static void EnsureSampleTube(Flawy::Assets::AssetManager& oAssetManager);
        static void AddModels(ImportResources& oResources, const Flawy::Assets::AssetImportResult& oResult, const glm::mat4& matModel);
        static void CreateAnimator(ImportResources& oResources,
                                   Flawy::Assets::AssetHandle<Flawy::Animation::SkeletonAsset> oSkeleton,
                                   Flawy::Assets::AssetHandle<Flawy::Animation::AnimationClipAsset> oClip,
                                   const glm::mat4& matJointOrigin);
    };
}
