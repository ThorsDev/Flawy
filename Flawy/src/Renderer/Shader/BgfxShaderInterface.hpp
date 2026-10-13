#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace Flawy::Renderer
{
    struct BgfxUniform
    {
        std::string sName;
        uint8_t uType = 0;
        uint8_t uNum = 0;
        uint16_t uRegIndex = 0;
        uint16_t uRegCount = 0;
        uint8_t uTexComponent = 0;
        uint8_t uTexDimension = 0;
        uint16_t uTexFormat = 0;
    };

    struct BgfxShaderInterface
    {
        std::vector<BgfxUniform> vUniforms;
        std::vector<uint16_t> vAttributeIds;
        uint16_t uUniformBufferSize = 0;
    };
}
