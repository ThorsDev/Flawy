#pragma once

#include "Flawy/Renderer/RenderGraph/RenderGraphResource.hpp"

#include <memory>

namespace Flawy::Renderer
{
    class IRenderPass;
    class RenderDevice;
    class RenderTarget;
    struct RenderTargetDescriptor;

    class RenderGraph final
    {
    public:
        struct Impl;

        explicit RenderGraph(RenderDevice& oDevice);
        ~RenderGraph();

        RenderGraph(const RenderGraph&) = delete;
        RenderGraph& operator=(const RenderGraph&) = delete;
        RenderGraph(RenderGraph&&) = delete;
        RenderGraph& operator=(RenderGraph&&) = delete;

        void BeginFrame();
        void AddPass(IRenderPass& oPass);
        [[nodiscard]] RenderGraphResource GetBackbuffer() const noexcept;
        [[nodiscard]] RenderGraphResource ImportTarget(RenderTarget& oTarget);
        [[nodiscard]] RenderGraphResource CreateTransientTarget(const RenderTargetDescriptor& oDescriptor);
        void Execute();

    private:
        std::unique_ptr<Impl> m_upImpl;
    };
}
