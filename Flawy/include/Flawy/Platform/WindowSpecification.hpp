#pragma once

#include <cstdint>
#include <string>

namespace Flawy::Platform
{
    struct WindowSpecification
    {
        std::string sTitle = "Flawy";
        uint32_t uWidth = 1280;
        uint32_t uHeight = 720;
        bool bResizable = true;
    };
}
