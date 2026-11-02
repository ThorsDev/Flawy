#pragma once

#include "SceneResources.hpp"

#include <Flawy/Flawy.hpp>

namespace Sandbox
{
    class SceneFactory final
    {
    public:
        SceneFactory() = delete;

        static void Create(SceneResources& oResources, Flawy::Renderer::MaterialSystem& oMaterialSystem, Flawy::Assets::AssetManager& oAssetManager);

    private:
        static void CreateMeshes(SceneResources& oResources, Flawy::Assets::AssetManager& oAssetManager);
        static void EnsureCubeAsset(Flawy::Assets::AssetManager& oAssetManager);
        static void CreateTextures(SceneResources& oResources);
        static void CreateMaterial(SceneResources& oResources, Flawy::Renderer::MaterialSystem& oMaterialSystem);
        static void CreateMaterialInstances(SceneResources& oResources);
    };
}
