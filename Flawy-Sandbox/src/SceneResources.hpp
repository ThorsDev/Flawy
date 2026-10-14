#pragma once

#include <Flawy/Flawy.hpp>

#include <memory>

namespace Sandbox
{
    struct SceneResources
    {
        std::unique_ptr<Flawy::Renderer::ShaderProgram> upProgram;
        std::unique_ptr<Flawy::Renderer::VertexBuffer> upVertexBuffer;
        std::unique_ptr<Flawy::Renderer::IndexBuffer> upIndexBuffer;
        std::unique_ptr<Flawy::Renderer::Texture> upTexture;
        std::unique_ptr<Flawy::Renderer::ShaderParameters> upParameters;
    };
}
