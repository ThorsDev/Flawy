#pragma once

#include "Flawy/Renderer/Shader/ShaderSource.hpp"

#include <optional>
#include <string_view>

namespace Flawy::Renderer
{
    class IShaderSourceProvider
    {
    public:
        virtual ~IShaderSourceProvider() = default;

        [[nodiscard]] virtual std::optional<ShaderSource> Load(std::string_view svPath) const = 0;
    };
}
