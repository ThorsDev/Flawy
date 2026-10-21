#include "Flawy/Renderer/RenderGraph/RenderPassContext.hpp"

#include "Flawy/Renderer/LightEnvironment.hpp"
#include "Flawy/Renderer/Material/Material.hpp"
#include "Flawy/Renderer/Material/MaterialInstance.hpp"
#include "Flawy/Renderer/Texture.hpp"
#include "Flawy/Renderer/VertexBuffer.hpp"
#include "Flawy/Renderer/ViewParameters.hpp"
#include "Renderer/Bgfx/BgfxDrawSubmitter.hpp"
#include "Renderer/Material/MaterialInstanceImpl.hpp"
#include "Renderer/Material/VertexFactoryTable.hpp"
#include "Renderer/RenderGraph/RenderGraphImpl.hpp"

#include <bgfx/bgfx.h>
#include <glm/gtc/type_ptr.hpp>

#include <format>
#include <stdexcept>

namespace Flawy::Renderer
{
    namespace
    {
        DrawCommand ResolveMaterialDraw(const DrawCommand& oCommand, MaterialPass ePass)
        {
            if (oCommand.pProgram != nullptr || oCommand.pParameters != nullptr)
            {
                throw std::invalid_argument("DrawCommand sets both a material and a raw shader program");
            }
            if (oCommand.pVertexBuffer == nullptr)
            {
                throw std::invalid_argument("DrawCommand requires a vertex buffer");
            }

            const MaterialInstance& oInstance = *oCommand.pMaterial;
            MaterialPermutationKey oKey;
            oKey.ePass = ePass;
            oKey.eVertexFactory = VertexFactoryTable::FromLayout(oCommand.pVertexBuffer->GetLayout());
            oKey.uStaticSwitchSet = oInstance.GetImpl().uStaticSwitchSet;
            const MaterialPermutation& oPermutation = oInstance.GetMaterial().CompilePermutation(oKey);

            DrawCommand oResolved = oCommand;
            oResolved.pMaterial = nullptr;
            oResolved.pProgram = oPermutation.pProgram;
            oResolved.pParameters = &oInstance.GetImpl().AcquireParameters(oPermutation);
            return oResolved;
        }
    }

    RenderPassContext::RenderPassContext(RenderGraph::Impl& oGraph) noexcept
        : m_oGraph(oGraph)
    {
    }

    void RenderPassContext::SetViewTransform(const glm::mat4& matView, const glm::mat4& matProjection)
    {
        const uint16_t uPassIndex = m_oGraph.RequireExecutePass("SetViewTransform");
        bgfx::setViewTransform(m_oGraph.vPasses[uPassIndex].uViewId, glm::value_ptr(matView), glm::value_ptr(matProjection));
    }

    void RenderPassContext::SetView(const ViewParameters& oView)
    {
        const uint16_t uPassIndex = m_oGraph.RequireExecutePass("SetView");
        const RenderGraph::Impl::PassRecord& oPass = m_oGraph.vPasses[uPassIndex];
        const RenderGraph::Impl::ResourceRecord& oTarget = m_oGraph.vResources[oPass.oWrite.uIndex];
        bgfx::setViewTransform(oPass.uViewId, glm::value_ptr(oView.matView), glm::value_ptr(oView.matProjection));
        m_oGraph.oViewUniforms.SetView(oView, oTarget.uWidth, oTarget.uHeight);
    }

    void RenderPassContext::SetLights(const LightEnvironment& oLights)
    {
        static_cast<void>(m_oGraph.RequireExecutePass("SetLights"));
        m_oGraph.oViewUniforms.SetLights(oLights);
    }

    void RenderPassContext::Draw(const DrawCommand& oCommand)
    {
        const uint16_t uPassIndex = m_oGraph.RequireExecutePass("Draw");
        const RenderGraph::Impl::PassRecord& oPass = m_oGraph.vPasses[uPassIndex];
        if (oCommand.pMaterial == nullptr)
        {
            m_oGraph.oViewUniforms.Apply();
            BgfxDrawSubmitter::Submit(oPass.uViewId, oCommand);
            return;
        }

        const DrawCommand oResolved = ResolveMaterialDraw(oCommand, oPass.eMaterialPass);
        m_oGraph.oViewUniforms.Apply();
        BgfxDrawSubmitter::Submit(oPass.uViewId, oResolved);
    }

    const Texture& RenderPassContext::GetTexture(RenderGraphResource oTarget) const
    {
        const uint16_t uPassIndex = m_oGraph.RequireExecutePass("GetTexture");
        const RenderGraph::Impl::PassRecord& oPass = m_oGraph.vPasses[uPassIndex];
        m_oGraph.RequireReadable(oTarget, oPass);
        if (!m_oGraph.IsDeclaredRead(oPass, oTarget))
        {
            throw std::logic_error(std::format("Pass '{}' samples {} without declaring it with RenderPassBuilder::ReadTexture", oPass.GetName(), m_oGraph.DescribeResource(oTarget)));
        }
        return m_oGraph.vResources[oTarget.uIndex].pTarget->GetColorTexture();
    }

    uint32_t RenderPassContext::GetTargetWidth() const
    {
        const uint16_t uPassIndex = m_oGraph.RequireExecutePass("GetTargetWidth");
        return m_oGraph.vResources[m_oGraph.vPasses[uPassIndex].oWrite.uIndex].uWidth;
    }

    uint32_t RenderPassContext::GetTargetHeight() const
    {
        const uint16_t uPassIndex = m_oGraph.RequireExecutePass("GetTargetHeight");
        return m_oGraph.vResources[m_oGraph.vPasses[uPassIndex].oWrite.uIndex].uHeight;
    }

    std::string_view RenderPassContext::GetPassName() const
    {
        const uint16_t uPassIndex = m_oGraph.RequireExecutePass("GetPassName");
        return m_oGraph.vPasses[uPassIndex].GetName();
    }
}
