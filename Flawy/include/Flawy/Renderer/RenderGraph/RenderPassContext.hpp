#pragma once

#include "Flawy/Renderer/DrawCommand.hpp"
#include "Flawy/Renderer/RenderGraph/RenderGraph.hpp"
#include "Flawy/Renderer/RenderGraph/RenderGraphResource.hpp"

#include <glm/mat4x4.hpp>

#include <cstdint>
#include <string_view>

namespace Flawy::Renderer
{
    class Texture;

    class RenderPassContext final
    {
    public:
        RenderPassContext(const RenderPassContext&) = delete;
        RenderPassContext& operator=(const RenderPassContext&) = delete;
        RenderPassContext(RenderPassContext&&) = delete;
        RenderPassContext& operator=(RenderPassContext&&) = delete;
        ~RenderPassContext() = default;

        void SetViewTransform(const glm::mat4& matView, const glm::mat4& matProjection);
        void Draw(const DrawCommand& oCommand);

        [[nodiscard]] const Texture& GetTexture(RenderGraphResource oTarget) const;
        [[nodiscard]] uint32_t GetTargetWidth() const;
        [[nodiscard]] uint32_t GetTargetHeight() const;
        [[nodiscard]] std::string_view GetPassName() const;

    private:
        friend struct RenderGraph::Impl;

        explicit RenderPassContext(RenderGraph::Impl& oGraph) noexcept;

        RenderGraph::Impl& m_oGraph;
    };
}
