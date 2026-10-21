#include "DepthPrePass.hpp"

#include "SceneDrawer.hpp"

#include <string_view>

namespace Sandbox
{
    namespace
    {
        constexpr std::string_view kPassName = "DepthPrePass";
    }

    DepthPrePass::DepthPrePass(const SceneResources& oResources) noexcept
        : m_oResources(oResources)
    {
    }

    void DepthPrePass::SetTarget(Flawy::Renderer::RenderGraphResource oTarget) noexcept
    {
        m_oTarget = oTarget;
    }

    void DepthPrePass::SetTimeSeconds(float fTimeSeconds) noexcept
    {
        m_fTimeSeconds = fTimeSeconds;
    }

    void DepthPrePass::Setup(Flawy::Renderer::RenderPassBuilder& oBuilder)
    {
        Flawy::Renderer::RenderPassClear oClear;
        oClear.bClearColor = false;
        oClear.bClearStencil = true;

        oBuilder.SetName(kPassName);
        oBuilder.SetMaterialPass(Flawy::Renderer::MaterialPass::DepthOnly);
        oBuilder.WriteTarget(m_oTarget, oClear);
    }

    void DepthPrePass::Execute(Flawy::Renderer::RenderPassContext& oContext)
    {
        SceneDrawer::ApplyView(oContext, m_fTimeSeconds);

        Flawy::Renderer::RenderState oState;
        oState.eDepthTest = Flawy::Renderer::DepthTest::Less;
        oState.bDepthWrite = true;
        oState.bColorWrite = false;
        SceneDrawer::DrawMeshes(oContext, m_oResources, m_fTimeSeconds, oState);
    }
}
