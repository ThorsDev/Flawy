#pragma once

#include "Flawy/Renderer/VertexLayout.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace Flawy::Renderer
{
    enum class ShaderUniformType : uint8_t
    {
        Vec4,
        Mat3,
        Mat4,
        Sampler
    };

    struct ShaderUniform
    {
        std::string sName;
        ShaderUniformType eType = ShaderUniformType::Vec4;
        uint16_t uCount = 1;
        uint8_t uTextureStage = 0;
        bool bPredefined = false;
    };

    struct ShaderInterface
    {
        std::vector<ShaderUniform> vUniforms;
        std::vector<VertexAttribute> vVertexAttributes;
        uint32_t uUniformBufferSize = 0;
    };
}
