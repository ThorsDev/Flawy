#pragma once

#include <string>
#include <vector>

namespace Flawy::Renderer
{
    struct ShaderDefine
    {
        std::string sName;
        std::string sValue;
    };

    struct ShaderSource
    {
        std::string sName;
        std::string sPath;
        std::string sCode;
        std::vector<ShaderDefine> vDefines;
    };
}
