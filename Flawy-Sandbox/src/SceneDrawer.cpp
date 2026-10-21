#include "SceneDrawer.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

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
        DrawMesh(oContext, *oResources.upCubeVertexBuffer, *oResources.upCubeIndexBuffer, *oResources.upCubeInstance, BuildCubeTransform(fTimeSeconds), oState);
        DrawMesh(oContext, *oResources.upCubeVertexBuffer, *oResources.upCubeIndexBuffer, *oResources.upAccentInstance, BuildAccentTransform(fTimeSeconds), oState);
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
