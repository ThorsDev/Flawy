#pragma once

#include "Flawy/Renderer/Material/MaterialPass.hpp"
#include "Flawy/Renderer/Shader/ShaderModuleSource.hpp"

#include <string_view>

namespace Flawy::Renderer
{
    class MaterialPassTable final
    {
    public:
        MaterialPassTable() = delete;

        [[nodiscard]] static std::string_view GetName(MaterialPass ePass) noexcept;
        [[nodiscard]] static ShaderModuleSource LoadModule(MaterialPass ePass);
    };
}
