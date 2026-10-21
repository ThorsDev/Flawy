#pragma once

#include "SceneResources.hpp"

#include <Flawy/Flawy.hpp>

namespace Sandbox
{
    class SceneFactory final
    {
    public:
        SceneFactory() = delete;

        static void Create(SceneResources& oResources, Flawy::Renderer::MaterialSystem& oMaterialSystem);

    private:
        static void CreateMeshes(SceneResources& oResources);
        static void CreateTextures(SceneResources& oResources);
        static void CreateMaterial(SceneResources& oResources, Flawy::Renderer::MaterialSystem& oMaterialSystem);
        static void CreateMaterialInstances(SceneResources& oResources);
    };
}
