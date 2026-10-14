#include "ScenePass.hpp"

#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

#include <cstdint>
#include <string_view>

namespace Sandbox
{
    namespace
    {
        constexpr std::string_view kPassName = "Scene";
        constexpr uint32_t kClearColorRgba = 0x202840ffu;
        constexpr glm::vec3 kCameraPosition = {0.0f, 1.2f, 2.6f};
        constexpr float kFieldOfViewDegrees = 60.0f;
        constexpr float kNearPlane = 0.1f;
        constexpr float kFarPlane = 100.0f;
    }

    ScenePass::ScenePass(const SceneResources& oResources) noexcept
        : m_oResources(oResources)
    {
    }

    void ScenePass::SetTarget(Flawy::Renderer::RenderGraphResource oTarget) noexcept
    {
        m_oTarget = oTarget;
    }

    void ScenePass::SetModelMatrix(const glm::mat4& matModel) noexcept
    {
        m_matModel = matModel;
    }

    void ScenePass::Setup(Flawy::Renderer::RenderPassBuilder& oBuilder)
    {
        Flawy::Renderer::RenderPassClear oClear;
        oClear.uColorRgba = kClearColorRgba;
        oClear.bClearStencil = true;

        oBuilder.SetName(kPassName);
        oBuilder.WriteTarget(m_oTarget, oClear);
    }

    void ScenePass::Execute(Flawy::Renderer::RenderPassContext& oContext)
    {
        const float fAspect = static_cast<float>(oContext.GetTargetWidth()) / static_cast<float>(oContext.GetTargetHeight());
        const glm::mat4 matView = glm::lookAtRH(kCameraPosition, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        const glm::mat4 matProjection = glm::perspectiveRH_ZO(glm::radians(kFieldOfViewDegrees), fAspect, kNearPlane, kFarPlane);
        oContext.SetViewTransform(matView, matProjection);

        Flawy::Renderer::DrawCommand oCommand;
        oCommand.pVertexBuffer = m_oResources.upVertexBuffer.get();
        oCommand.pIndexBuffer = m_oResources.upIndexBuffer.get();
        oCommand.pProgram = m_oResources.upProgram.get();
        oCommand.pParameters = m_oResources.upParameters.get();
        oCommand.matModel = m_matModel;
        oContext.Draw(oCommand);
    }
}
