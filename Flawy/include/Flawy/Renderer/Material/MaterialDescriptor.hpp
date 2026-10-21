#pragma once

#include "Flawy/Renderer/Material/MaterialParameterLayout.hpp"

#include <string>

namespace Flawy::Renderer
{
    struct MaterialDescriptor
    {
        std::string sModuleName;
        std::string sSlangSource;
        std::string sSourcePath;
        MaterialParameterLayout oParameters;
        std::string sDebugName;
    };
}
