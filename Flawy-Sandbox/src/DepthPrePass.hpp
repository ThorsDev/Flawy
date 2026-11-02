#pragma once

#include "ImportResources.hpp"
#include "SceneResources.hpp"

#include <Flawy/Flawy.hpp>

#include <glm/mat4x4.hpp>

#include <span>

namespace Sandbox
{
    class DepthPrePass final : public Flawy::Renderer::IRenderPass
    {
    public:
        DepthPrePass(const SceneResources& oResources, const ImportResources& oImportResources) noexcept;

        void SetTarget(Flawy::Renderer::RenderGraphResource oTarget) noexcept;
        void SetTimeSeconds(float fTimeSeconds) noexcept;
        void SetJointMatrices(std::span<const glm::mat4> spanJointMatrices) noexcept;

        void Setup(Flawy::Renderer::RenderPassBuilder& oBuilder) override;
        void Execute(Flawy::Renderer::RenderPassContext& oContext) override;

    private:
        const SceneResources& m_oResources;
        const ImportResources& m_oImportResources;
        Flawy::Renderer::RenderGraphResource m_oTarget;
        float m_fTimeSeconds = 0.0f;
        std::span<const glm::mat4> m_spanJointMatrices;
    };
}
