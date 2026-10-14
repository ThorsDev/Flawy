#include "PostProcessPass.hpp"

#include <string_view>

namespace Sandbox
{
    namespace
    {
        constexpr std::string_view kPassName = "PostProcess";
        constexpr std::string_view kTextureParameter = "s_texColor";
        constexpr std::string_view kTintParameter = "u_tint";
    }

    PostProcessPass::PostProcessPass(PostProcessResources& oResources) noexcept
        : m_oResources(oResources)
    {
    }

    void PostProcessPass::SetSource(Flawy::Renderer::RenderGraphResource oSource) noexcept
    {
        m_oSource = oSource;
    }

    void PostProcessPass::SetTarget(Flawy::Renderer::RenderGraphResource oTarget) noexcept
    {
        m_oTarget = oTarget;
    }

    void PostProcessPass::SetTint(const glm::vec4& vecTint) noexcept
    {
        m_vecTint = vecTint;
    }

    void PostProcessPass::Setup(Flawy::Renderer::RenderPassBuilder& oBuilder)
    {
        oBuilder.SetName(kPassName);
        oBuilder.ReadTexture(m_oSource);
        oBuilder.WriteTarget(m_oTarget, Flawy::Renderer::RenderPassClear::None());
    }

    void PostProcessPass::Execute(Flawy::Renderer::RenderPassContext& oContext)
    {
        Flawy::Renderer::ShaderParameters& oParameters = *m_oResources.upParameters;
        oParameters.SetTexture(oParameters.Require(kTextureParameter), oContext.GetTexture(m_oSource));
        oParameters.SetVec4(oParameters.Require(kTintParameter), m_vecTint);

        Flawy::Renderer::DrawCommand oCommand;
        oCommand.pVertexBuffer = m_oResources.upVertexBuffer.get();
        oCommand.pIndexBuffer = m_oResources.upIndexBuffer.get();
        oCommand.pProgram = m_oResources.upProgram.get();
        oCommand.pParameters = &oParameters;
        oCommand.oState.eDepthTest = Flawy::Renderer::DepthTest::Disabled;
        oCommand.oState.bDepthWrite = false;
        oCommand.oState.eCull = Flawy::Renderer::CullMode::None;
        oContext.Draw(oCommand);
    }
}
