#include "SceneDrawer.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

#include <cstdint>

namespace Sandbox
{
    namespace
    {
        constexpr glm::vec3 kCameraPosition = {0.0f, 1.6f, 3.2f};
        constexpr float kFieldOfViewDegrees = 60.0f;
        constexpr float kNearPlane = 0.1f;
        constexpr float kFarPlane = 100.0f;
        constexpr float kYawRadiansPerSecond = 0.9f;
        constexpr float kPitchRadiansPerSecond = 0.5f;
        constexpr float kAccentYawRadiansPerSecond = 0.4f;
        constexpr glm::vec3 kAccentPosition = {1.1f, -0.65f, 0.6f};
        constexpr float kAccentScale = 0.5f;
        constexpr glm::vec3 kYAxis = {0.0f, 1.0f, 0.0f};
        constexpr glm::vec3 kXAxis = {1.0f, 0.0f, 0.0f};
        constexpr glm::vec3 kSkeletonOrigin = {-1.4f, -0.9f, 0.0f};
        constexpr float kJointCubeScale = 0.08f;

        glm::mat4 BuildCubeTransform(float fTimeSeconds)
        {
            const glm::mat4 matYaw = glm::rotate(glm::mat4(1.0f), fTimeSeconds * kYawRadiansPerSecond, kYAxis);
            return glm::rotate(matYaw, fTimeSeconds * kPitchRadiansPerSecond, kXAxis);
        }

        glm::mat4 BuildAccentTransform(float fTimeSeconds)
        {
            const glm::mat4 matPlaced = glm::rotate(glm::translate(glm::mat4(1.0f), kAccentPosition), fTimeSeconds * kAccentYawRadiansPerSecond, kYAxis);
            return glm::scale(matPlaced, glm::vec3(kAccentScale));
        }
    }

    void SceneDrawer::ApplyView(Flawy::Renderer::RenderPassContext& oContext, float fTimeSeconds)
    {
        const float fAspect = static_cast<float>(oContext.GetTargetWidth()) / static_cast<float>(oContext.GetTargetHeight());

        Flawy::Renderer::ViewParameters oView;
        oView.matView = glm::lookAtRH(kCameraPosition, glm::vec3(0.0f), kYAxis);
        oView.matProjection = glm::perspectiveRH_ZO(glm::radians(kFieldOfViewDegrees), fAspect, kNearPlane, kFarPlane);
        oView.vecCameraPosition = kCameraPosition;
        oView.fTimeSeconds = fTimeSeconds;
        oContext.SetView(oView);
    }

    void SceneDrawer::DrawMeshes(Flawy::Renderer::RenderPassContext& oContext, const SceneResources& oResources, float fTimeSeconds, const Flawy::Renderer::RenderState& oState)
    {
        DrawMesh(oContext, *oResources.upGroundVertexBuffer, *oResources.upGroundIndexBuffer, *oResources.upGroundInstance, glm::mat4(1.0f), oState);
        DrawStaticMesh(oContext, oResources.oCubeMesh.Get(), *oResources.upCubeInstance, BuildCubeTransform(fTimeSeconds), oState);
        DrawStaticMesh(oContext, oResources.oCubeMesh.Get(), *oResources.upAccentInstance, BuildAccentTransform(fTimeSeconds), oState);
    }

    void SceneDrawer::DrawSkeletonJoints(Flawy::Renderer::RenderPassContext& oContext,
                                         const SceneResources& oResources,
                                         std::span<const glm::mat4> spanJointMatrices,
                                         const Flawy::Renderer::RenderState& oState)
    {
        DrawJoints(oContext, oResources, spanJointMatrices, glm::translate(glm::mat4(1.0f), kSkeletonOrigin), oState);
    }

    void SceneDrawer::DrawImported(Flawy::Renderer::RenderPassContext& oContext,
                                   const SceneResources& oSceneResources,
                                   const ImportResources& oImportResources,
                                   const Flawy::Renderer::RenderState& oState)
    {
        for (const ImportedModelInstance& oModel : oImportResources.vModels)
        {
            DrawStaticMesh(oContext, oModel.oMesh.Get(), *oSceneResources.upCubeInstance, oModel.matModel, oState);
        }
        if (oImportResources.upAnimator)
        {
            DrawJoints(oContext, oSceneResources, oImportResources.upAnimator->GetModelPose().GetJointMatrices(), oImportResources.matJointOrigin, oState);
        }
    }

    void SceneDrawer::DrawJoints(Flawy::Renderer::RenderPassContext& oContext,
                                 const SceneResources& oResources,
                                 std::span<const glm::mat4> spanJointMatrices,
                                 const glm::mat4& matOrigin,
                                 const Flawy::Renderer::RenderState& oState)
    {
        for (const glm::mat4& matJoint : spanJointMatrices)
        {
            const glm::mat4 matModel = glm::scale(matOrigin * matJoint, glm::vec3(kJointCubeScale));
            DrawStaticMesh(oContext, oResources.oCubeMesh.Get(), *oResources.upAccentInstance, matModel, oState);
        }
    }

    void SceneDrawer::DrawStaticMesh(Flawy::Renderer::RenderPassContext& oContext,
                                     const Flawy::Assets::StaticMeshAsset& oMesh,
                                     const Flawy::Renderer::MaterialInstance& oMaterial,
                                     const glm::mat4& matModel,
                                     const Flawy::Renderer::RenderState& oState)
    {
        for (uint32_t uSection = 0; uSection < oMesh.GetSectionCount(); ++uSection)
        {
            DrawMesh(oContext, oMesh.GetVertexBuffer(), oMesh.GetSectionIndexBuffer(uSection), oMaterial, matModel, oState);
        }
    }

    void SceneDrawer::DrawMesh(Flawy::Renderer::RenderPassContext& oContext,
                               const Flawy::Renderer::VertexBuffer& oVertexBuffer,
                               const Flawy::Renderer::IndexBuffer& oIndexBuffer,
                               const Flawy::Renderer::MaterialInstance& oMaterial,
                               const glm::mat4& matModel,
                               const Flawy::Renderer::RenderState& oState)
    {
        Flawy::Renderer::DrawCommand oCommand;
        oCommand.pVertexBuffer = &oVertexBuffer;
        oCommand.pIndexBuffer = &oIndexBuffer;
        oCommand.pMaterial = &oMaterial;
        oCommand.oState = oState;
        oCommand.matModel = matModel;
        oContext.Draw(oCommand);
    }
}
