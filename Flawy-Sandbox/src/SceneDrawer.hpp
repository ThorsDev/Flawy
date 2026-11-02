#pragma once

#include "ImportResources.hpp"
#include "SceneResources.hpp"

#include <Flawy/Flawy.hpp>

#include <glm/mat4x4.hpp>

#include <span>

namespace Sandbox
{
    class SceneDrawer final
    {
    public:
        SceneDrawer() = delete;

        static void ApplyView(Flawy::Renderer::RenderPassContext& oContext, float fTimeSeconds);
        static void DrawMeshes(Flawy::Renderer::RenderPassContext& oContext, const SceneResources& oResources, float fTimeSeconds, const Flawy::Renderer::RenderState& oState);
        static void DrawSkeletonJoints(Flawy::Renderer::RenderPassContext& oContext,
                                       const SceneResources& oResources,
                                       std::span<const glm::mat4> spanJointMatrices,
                                       const Flawy::Renderer::RenderState& oState);
        static void DrawImported(Flawy::Renderer::RenderPassContext& oContext,
                                 const SceneResources& oSceneResources,
                                 const ImportResources& oImportResources,
                                 const Flawy::Renderer::RenderState& oState);

    private:
        static void DrawJoints(Flawy::Renderer::RenderPassContext& oContext,
                               const SceneResources& oResources,
                               std::span<const glm::mat4> spanJointMatrices,
                               const glm::mat4& matOrigin,
                               const Flawy::Renderer::RenderState& oState);
        static void DrawStaticMesh(Flawy::Renderer::RenderPassContext& oContext,
                                   const Flawy::Assets::StaticMeshAsset& oMesh,
                                   const Flawy::Renderer::MaterialInstance& oMaterial,
                                   const glm::mat4& matModel,
                                   const Flawy::Renderer::RenderState& oState);
        static void DrawMesh(Flawy::Renderer::RenderPassContext& oContext,
                             const Flawy::Renderer::VertexBuffer& oVertexBuffer,
                             const Flawy::Renderer::IndexBuffer& oIndexBuffer,
                             const Flawy::Renderer::MaterialInstance& oMaterial,
                             const glm::mat4& matModel,
                             const Flawy::Renderer::RenderState& oState);
    };
}
