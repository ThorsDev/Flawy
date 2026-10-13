#pragma once

#include "Flawy/Renderer/RendererBackend.hpp"
#include "Renderer/Shader/IShaderBackendEmitter.hpp"

#include <memory>

namespace Flawy::Renderer
{
    class ShaderBackendEmitterFactory final
    {
    public:
        ShaderBackendEmitterFactory() = delete;

        [[nodiscard]] static std::unique_ptr<IShaderBackendEmitter> Create(RendererBackend eBackend);
    };
}
