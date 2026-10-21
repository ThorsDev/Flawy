#pragma once

#include "Flawy/Renderer/Shader/ShaderModuleSource.hpp"
#include "Flawy/Renderer/Shader/ShaderStage.hpp"

#include <string>
#include <vector>

namespace Flawy::Renderer
{
    struct SpecializedShaderRequest
    {
        ShaderModuleSource oPassModule;
        std::string sEntryPoint;
        ShaderStage eStage = ShaderStage::Vertex;
        std::vector<ShaderModuleSource> vModules;
        std::vector<std::string> vTypeArguments;
    };
}
