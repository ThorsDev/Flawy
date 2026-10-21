#pragma once

#include "SceneResources.hpp"

#include <Flawy/Flawy.hpp>

#include <glm/mat4x4.hpp>

namespace Sandbox
{
    class SceneDrawer final
    {
    public:
        SceneDrawer() = delete;

        static void ApplyView(Flawy::Renderer::RenderPassContext& oContext, float fTimeSeconds);
        static void DrawMeshes(Flawy::Renderer::RenderPassContext& oContext, const SceneResources& oResources, float fTimeSeconds, const Flawy::Renderer::RenderState& oState);

    private:
        static void DrawMesh(Flawy::Renderer::RenderPassContext& oContext,
                             const Flawy::Renderer::VertexBuffer& oVertexBuffer,
                             const Flawy::Renderer::IndexBuffer& oIndexBuffer,
                             const Flawy::Renderer::MaterialInstance& oMaterial,
                             const glm::mat4& matModel,
                             const Flawy::Renderer::RenderState& oState);
    };
}
