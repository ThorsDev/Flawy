#pragma once

#include "Flawy/Renderer/DrawCommand.hpp"

#include <bgfx/bgfx.h>

namespace Flawy::Renderer
{
    class BgfxDrawSubmitter final
    {
    public:
        BgfxDrawSubmitter() = delete;

        static void Submit(bgfx::ViewId uViewId, const DrawCommand& oCommand);

    private:
        static void Validate(const DrawCommand& oCommand);
    };
}
