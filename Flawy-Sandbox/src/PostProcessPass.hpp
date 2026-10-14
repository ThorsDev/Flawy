#pragma once

#include "PostProcessResources.hpp"

#include <Flawy/Flawy.hpp>

#include <glm/vec4.hpp>

namespace Sandbox
{
    class PostProcessPass final : public Flawy::Renderer::IRenderPass
    {
    public:
        explicit PostProcessPass(PostProcessResources& oResources) noexcept;

        void SetSource(Flawy::Renderer::RenderGraphResource oSource) noexcept;
        void SetTarget(Flawy::Renderer::RenderGraphResource oTarget) noexcept;
        void SetTint(const glm::vec4& vecTint) noexcept;

        void Setup(Flawy::Renderer::RenderPassBuilder& oBuilder) override;
        void Execute(Flawy::Renderer::RenderPassContext& oContext) override;

    private:
        PostProcessResources& m_oResources;
        Flawy::Renderer::RenderGraphResource m_oSource;
        Flawy::Renderer::RenderGraphResource m_oTarget;
        glm::vec4 m_vecTint{1.0f};
    };
}
