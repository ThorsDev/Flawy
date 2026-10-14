#include "Flawy/Renderer/RenderGraph/RenderPassContext.hpp"

#include "Flawy/Renderer/Texture.hpp"
#include "Renderer/Bgfx/BgfxDrawSubmitter.hpp"
#include "Renderer/RenderGraph/RenderGraphImpl.hpp"

#include <bgfx/bgfx.h>
#include <glm/gtc/type_ptr.hpp>

#include <format>
#include <stdexcept>

namespace Flawy::Renderer
{
    RenderPassContext::RenderPassContext(RenderGraph::Impl& oGraph) noexcept
        : m_oGraph(oGraph)
    {
    }

    void RenderPassContext::SetViewTransform(const glm::mat4& matView, const glm::mat4& matProjection)
    {
        const uint16_t uPassIndex = m_oGraph.RequireExecutePass("SetViewTransform");
        bgfx::setViewTransform(m_oGraph.vPasses[uPassIndex].uViewId, glm::value_ptr(matView), glm::value_ptr(matProjection));
    }

    void RenderPassContext::Draw(const DrawCommand& oCommand)
    {
        const uint16_t uPassIndex = m_oGraph.RequireExecutePass("Draw");
        BgfxDrawSubmitter::Submit(m_oGraph.vPasses[uPassIndex].uViewId, oCommand);
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
