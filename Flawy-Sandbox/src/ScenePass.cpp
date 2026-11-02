#include "ScenePass.hpp"

#include "SceneDrawer.hpp"

#include <glm/vec3.hpp>

#include <cstdint>
#include <string_view>

namespace Sandbox
{
    namespace
    {
        constexpr std::string_view kPassName = "Scene";
        constexpr uint32_t kClearColorRgba = 0x0a0d14ffu;
        constexpr glm::vec3 kSunDirection = {-0.5f, -1.0f, -0.3f};
        constexpr float kSunIntensity = 1.75f;
        constexpr float kPointLightRadius = 4.0f;
        constexpr float kPointLightIntensity = 15.0f;
        constexpr glm::vec3 kRedLightPosition = {1.4f, 0.2f, -0.15f};
        constexpr glm::vec3 kBlueLightPosition = {-1.5f, 0.3f, 1.0f};
        constexpr glm::vec3 kSkyAmbient = {0.25f, 0.3f, 0.4f};
        constexpr glm::vec3 kGroundAmbient = {0.08f, 0.06f, 0.05f};

        Flawy::Renderer::LightEnvironment BuildLights()
        {
            Flawy::Renderer::LightEnvironment oLights;
            oLights.optDirectional = Flawy::Renderer::DirectionalLight{kSunDirection, glm::vec3(1.0f), kSunIntensity};
            oLights.vPointLights.push_back({kRedLightPosition, kPointLightRadius, glm::vec3(1.0f, 0.1f, 0.05f), kPointLightIntensity});
            oLights.vPointLights.push_back({kBlueLightPosition, kPointLightRadius, glm::vec3(0.1f, 0.25f, 1.0f), kPointLightIntensity});
            oLights.vecSkyAmbient = kSkyAmbient;
            oLights.vecGroundAmbient = kGroundAmbient;
            return oLights;
        }
    }

    ScenePass::ScenePass(const SceneResources& oResources, const ImportResources& oImportResources) noexcept
        : m_oResources(oResources)
        , m_oImportResources(oImportResources)
    {
    }

    void ScenePass::SetTarget(Flawy::Renderer::RenderGraphResource oTarget) noexcept
    {
        m_oTarget = oTarget;
    }

    void ScenePass::SetTimeSeconds(float fTimeSeconds) noexcept
    {
        m_fTimeSeconds = fTimeSeconds;
    }

    void ScenePass::SetJointMatrices(std::span<const glm::mat4> spanJointMatrices) noexcept
    {
        m_spanJointMatrices = spanJointMatrices;
    }

    void ScenePass::Setup(Flawy::Renderer::RenderPassBuilder& oBuilder)
    {
        Flawy::Renderer::RenderPassClear oClear = Flawy::Renderer::RenderPassClear::None();
        oClear.bClearColor = true;
        oClear.uColorRgba = kClearColorRgba;

        oBuilder.SetName(kPassName);
        oBuilder.SetMaterialPass(Flawy::Renderer::MaterialPass::Forward);
        oBuilder.WriteTarget(m_oTarget, oClear);
    }

    void ScenePass::Execute(Flawy::Renderer::RenderPassContext& oContext)
    {
        SceneDrawer::ApplyView(oContext, m_fTimeSeconds);
        oContext.SetLights(BuildLights());

        Flawy::Renderer::RenderState oState;
        oState.eDepthTest = Flawy::Renderer::DepthTest::LessEqual;
        oState.bDepthWrite = false;
        SceneDrawer::DrawMeshes(oContext, m_oResources, m_fTimeSeconds, oState);
        SceneDrawer::DrawSkeletonJoints(oContext, m_oResources, m_spanJointMatrices, oState);
        SceneDrawer::DrawImported(oContext, m_oResources, m_oImportResources, oState);
    }
}
