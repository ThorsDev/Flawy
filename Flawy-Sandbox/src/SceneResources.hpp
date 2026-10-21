#pragma once

#include <Flawy/Flawy.hpp>

#include <memory>

namespace Sandbox
{
    struct SceneResources
    {
        std::unique_ptr<Flawy::Renderer::VertexBuffer> upCubeVertexBuffer;
        std::unique_ptr<Flawy::Renderer::IndexBuffer> upCubeIndexBuffer;
        std::unique_ptr<Flawy::Renderer::VertexBuffer> upGroundVertexBuffer;
        std::unique_ptr<Flawy::Renderer::IndexBuffer> upGroundIndexBuffer;
        std::unique_ptr<Flawy::Renderer::Texture> upCheckerboard;
        std::unique_ptr<Flawy::Renderer::Texture> upNormalMap;
        std::unique_ptr<Flawy::Renderer::Material> upMaterial;
        std::unique_ptr<Flawy::Renderer::MaterialInstance> upCubeInstance;
        std::unique_ptr<Flawy::Renderer::MaterialInstance> upGroundInstance;
        std::unique_ptr<Flawy::Renderer::MaterialInstance> upAccentInstance;
    };
}
