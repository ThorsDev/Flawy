#pragma once

#include "SceneResources.hpp"

#include <Flawy/Flawy.hpp>

#include <glm/mat4x4.hpp>

namespace Sandbox
{
    class ScenePass final : public Flawy::Renderer::IRenderPass
    {
    public:
        explicit ScenePass(const SceneResources& oResources) noexcept;

        void SetTarget(Flawy::Renderer::RenderGraphResource oTarget) noexcept;
        void SetModelMatrix(const glm::mat4& matModel) noexcept;

        void Setup(Flawy::Renderer::RenderPassBuilder& oBuilder) override;
        void Execute(Flawy::Renderer::RenderPassContext& oContext) override;

    private:
        const SceneResources& m_oResources;
        Flawy::Renderer::RenderGraphResource m_oTarget;
        glm::mat4 m_matModel{1.0f};
    };
}
