#pragma once

#include "SceneResources.hpp"

#include <Flawy/Flawy.hpp>

namespace Sandbox
{
    class DepthPrePass final : public Flawy::Renderer::IRenderPass
    {
    public:
        explicit DepthPrePass(const SceneResources& oResources) noexcept;

        void SetTarget(Flawy::Renderer::RenderGraphResource oTarget) noexcept;
        void SetTimeSeconds(float fTimeSeconds) noexcept;

        void Setup(Flawy::Renderer::RenderPassBuilder& oBuilder) override;
        void Execute(Flawy::Renderer::RenderPassContext& oContext) override;

    private:
        const SceneResources& m_oResources;
        Flawy::Renderer::RenderGraphResource m_oTarget;
        float m_fTimeSeconds = 0.0f;
    };
}
