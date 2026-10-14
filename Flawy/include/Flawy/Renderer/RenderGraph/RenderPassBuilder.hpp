#pragma once

#include "Flawy/Renderer/RenderGraph/RenderGraph.hpp"
#include "Flawy/Renderer/RenderGraph/RenderGraphResource.hpp"
#include "Flawy/Renderer/RenderGraph/RenderPassClear.hpp"

#include <string_view>

namespace Flawy::Renderer
{
    struct RenderTargetDescriptor;

    class RenderPassBuilder final
    {
    public:
        RenderPassBuilder(const RenderPassBuilder&) = delete;
        RenderPassBuilder& operator=(const RenderPassBuilder&) = delete;
        RenderPassBuilder(RenderPassBuilder&&) = delete;
        RenderPassBuilder& operator=(RenderPassBuilder&&) = delete;
        ~RenderPassBuilder() = default;

        void SetName(std::string_view svName);
        void WriteTarget(RenderGraphResource oTarget, const RenderPassClear& oClear);
        void ReadTexture(RenderGraphResource oTarget);
        void SetAlwaysExecute(bool bAlwaysExecute);
        [[nodiscard]] RenderGraphResource GetBackbuffer() const;
        [[nodiscard]] RenderGraphResource CreateTransientTarget(const RenderTargetDescriptor& oDescriptor);

    private:
        friend struct RenderGraph::Impl;

        explicit RenderPassBuilder(RenderGraph::Impl& oGraph) noexcept;

        RenderGraph::Impl& m_oGraph;
    };
}
