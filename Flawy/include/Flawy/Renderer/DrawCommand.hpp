#pragma once

#include "Flawy/Renderer/RenderState.hpp"

#include <glm/mat4x4.hpp>

namespace Flawy::Renderer
{
    class VertexBuffer;
    class IndexBuffer;
    class ShaderProgram;
    class ShaderParameters;

    struct DrawCommand
    {
        const VertexBuffer* pVertexBuffer = nullptr;
        const IndexBuffer* pIndexBuffer = nullptr;
        const ShaderProgram* pProgram = nullptr;
        const ShaderParameters* pParameters = nullptr;
        RenderState oState;
        glm::mat4 matModel{1.0f};
    };
}
