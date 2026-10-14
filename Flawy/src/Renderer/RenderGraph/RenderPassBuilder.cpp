#include "Flawy/Renderer/RenderGraph/RenderPassBuilder.hpp"

#include "Renderer/RenderGraph/RenderGraphImpl.hpp"

#include <algorithm>
#include <format>
#include <stdexcept>

namespace Flawy::Renderer
{
    RenderPassBuilder::RenderPassBuilder(RenderGraph::Impl& oGraph) noexcept
        : m_oGraph(oGraph)
    {
    }

    void RenderPassBuilder::SetName(std::string_view svName)
    {
        const uint16_t uPassIndex = m_oGraph.RequireSetupPass("SetName");
        if (svName.empty() || svName.size() > RenderGraph::Impl::kMaxPassNameLength)
        {
            throw std::invalid_argument(std::format("Render pass name '{}' must be between 1 and {} characters", svName, RenderGraph::Impl::kMaxPassNameLength));
        }

        RenderGraph::Impl::PassRecord& oPass = m_oGraph.vPasses[uPassIndex];
        oPass.aName.fill('\0');
        std::ranges::copy(svName, oPass.aName.begin());
    }

    void RenderPassBuilder::WriteTarget(RenderGraphResource oTarget, const RenderPassClear& oClear)
    {
        const uint16_t uPassIndex = m_oGraph.RequireSetupPass("WriteTarget");
        m_oGraph.RequireValidResource(oTarget);
        RenderGraph::Impl::PassRecord& oPass = m_oGraph.vPasses[uPassIndex];
        if (oPass.oWrite.IsValid())
        {
            throw std::logic_error(std::format("Pass '{}' already writes {}; a pass writes exactly one target", oPass.GetName(), m_oGraph.DescribeResource(oPass.oWrite)));
        }

        oPass.oWrite = oTarget;
        oPass.oClear = oClear;
    }

    void RenderPassBuilder::ReadTexture(RenderGraphResource oTarget)
    {
        const uint16_t uPassIndex = m_oGraph.RequireSetupPass("ReadTexture");
        RenderGraph::Impl::PassRecord& oPass = m_oGraph.vPasses[uPassIndex];
        m_oGraph.RequireReadable(oTarget, oPass);

        m_oGraph.vReads.push_back(oTarget);
        ++oPass.uReadCount;
    }

    void RenderPassBuilder::SetAlwaysExecute(bool bAlwaysExecute)
    {
        const uint16_t uPassIndex = m_oGraph.RequireSetupPass("SetAlwaysExecute");
        m_oGraph.vPasses[uPassIndex].bAlwaysExecute = bAlwaysExecute;
    }

    RenderGraphResource RenderPassBuilder::GetBackbuffer() const
    {
        static_cast<void>(m_oGraph.RequireSetupPass("GetBackbuffer"));
        return m_oGraph.oBackbuffer;
    }

    RenderGraphResource RenderPassBuilder::CreateTransientTarget(const RenderTargetDescriptor& oDescriptor)
    {
        static_cast<void>(m_oGraph.RequireSetupPass("CreateTransientTarget"));
        return m_oGraph.CreateTransient(oDescriptor);
    }
}
