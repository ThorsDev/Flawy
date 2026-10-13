#include "Renderer/Shader/ShaderBackendEmitterFactory.hpp"

#include "Renderer/Shader/DxbcShaderBackendEmitter.hpp"
#include "Renderer/Shader/SpirvShaderBackendEmitter.hpp"

#include <stdexcept>

namespace Flawy::Renderer
{
    std::unique_ptr<IShaderBackendEmitter> ShaderBackendEmitterFactory::Create(RendererBackend eBackend)
    {
        switch (eBackend)
        {
        case RendererBackend::Direct3D11:
            return std::make_unique<DxbcShaderBackendEmitter>();
        case RendererBackend::Vulkan:
            return std::make_unique<SpirvShaderBackendEmitter>();
        case RendererBackend::Default:
            break;
        }
        throw std::invalid_argument("ShaderCompiler requires a concrete renderer backend, not RendererBackend::Default");
    }
}
