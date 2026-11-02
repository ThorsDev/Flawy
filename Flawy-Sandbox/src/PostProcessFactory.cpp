#include "PostProcessFactory.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string_view>

namespace Sandbox
{
    namespace
    {
        constexpr std::string_view kPostProcessShaderName = "SandboxPostProcess";

        struct ScreenVertex
        {
            float fX;
            float fY;
            float fU;
            float fV;
        };

        constexpr std::array<ScreenVertex, 4> kScreenQuadVertices = {{
            {-1.0f, 1.0f, 0.0f, 0.0f},
            {1.0f, 1.0f, 1.0f, 0.0f},
            {1.0f, -1.0f, 1.0f, 1.0f},
            {-1.0f, -1.0f, 0.0f, 1.0f},
        }};
        constexpr std::array<uint16_t, 6> kScreenQuadIndices = {0, 2, 1, 0, 3, 2};
    }

    void PostProcessFactory::Create(PostProcessResources& oResources, Flawy::Renderer::ShaderLibrary& oLibrary)
    {
        oResources.pProgram = &oLibrary.GetProgram(kPostProcessShaderName);

        Flawy::Renderer::VertexLayout oLayout;
        oLayout.Add(Flawy::Renderer::VertexAttribute::Position, 2, Flawy::Renderer::VertexAttributeType::Float)
            .Add(Flawy::Renderer::VertexAttribute::TexCoord0, 2, Flawy::Renderer::VertexAttributeType::Float);
        oResources.upVertexBuffer = std::make_unique<Flawy::Renderer::VertexBuffer>(std::as_bytes(std::span(kScreenQuadVertices)), oLayout);
        oResources.upIndexBuffer = std::make_unique<Flawy::Renderer::IndexBuffer>(std::span<const uint16_t>(kScreenQuadIndices));
        oResources.upParameters = std::make_unique<Flawy::Renderer::ShaderParameters>(*oResources.pProgram);
    }
}
