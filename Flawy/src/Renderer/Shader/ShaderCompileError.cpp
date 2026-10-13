#include "Flawy/Renderer/Shader/ShaderCompileError.hpp"

namespace Flawy::Renderer
{
    ShaderCompileError::ShaderCompileError(const std::string& sMessage)
        : std::runtime_error(sMessage)
    {
    }
}
