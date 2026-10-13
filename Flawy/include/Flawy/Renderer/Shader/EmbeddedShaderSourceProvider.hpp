#pragma once

#include "Flawy/Renderer/Shader/IShaderSourceProvider.hpp"

#include <optional>
#include <string_view>
#include <vector>

namespace Flawy::Renderer
{
    class EmbeddedShaderSourceProvider final : public IShaderSourceProvider
    {
    public:
        [[nodiscard]] std::optional<ShaderSource> Load(std::string_view svPath) const override;
        [[nodiscard]] std::vector<std::string_view> ListPaths() const;
    };
}
