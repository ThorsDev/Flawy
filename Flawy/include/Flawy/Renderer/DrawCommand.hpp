#pragma once

#include <glm/mat4x4.hpp>

namespace Flawy::Renderer
{
    class VertexBuffer;
    class ShaderProgram;

    struct DrawCommand
    {
        const VertexBuffer* pVertexBuffer = nullptr;
        const ShaderProgram* pProgram = nullptr;
        glm::mat4 matModel{1.0f};
    };
}
