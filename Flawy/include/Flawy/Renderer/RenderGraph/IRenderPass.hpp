#pragma once

namespace Flawy::Renderer
{
    class RenderPassBuilder;
    class RenderPassContext;

    class IRenderPass
    {
    public:
        virtual ~IRenderPass() = default;

        virtual void Setup(RenderPassBuilder& oBuilder) = 0;
        virtual void Execute(RenderPassContext& oContext) = 0;
    };
}
