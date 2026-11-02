#pragma once

#include "PostProcessResources.hpp"

#include <Flawy/Flawy.hpp>

namespace Sandbox
{
    class PostProcessFactory final
    {
    public:
        PostProcessFactory() = delete;

        static void Create(PostProcessResources& oResources, Flawy::Renderer::ShaderLibrary& oLibrary);
    };
}
