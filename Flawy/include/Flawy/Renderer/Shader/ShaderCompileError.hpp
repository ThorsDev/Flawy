#pragma once

#include <stdexcept>
#include <string>

namespace Flawy::Renderer
{
    class ShaderCompileError final : public std::runtime_error
    {
    public:
        explicit ShaderCompileError(const std::string& sMessage);
    };
}
