#pragma once

#include "Flawy/Renderer/Shader/ShaderInterface.hpp"
#include "Flawy/Renderer/Shader/ShaderStage.hpp"

#include <cstddef>
#include <string>
#include <vector>

namespace Flawy::Renderer
{
    struct CompiledShader
    {
        ShaderStage eStage = ShaderStage::Vertex;
        std::string sName;
        std::string sEntryPoint;
        std::vector<std::byte> vBgfxBinary;
        ShaderInterface oInterface;
    };
}
