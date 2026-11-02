#pragma once

#include "Flawy/Renderer/Shader/ShaderInterface.hpp"

#include <string>
#include <string_view>

namespace Flawy::Renderer
{
    class ShaderInterfaceText final
    {
    public:
        ShaderInterfaceText() = delete;

        [[nodiscard]] static std::string_view GetTypeName(ShaderUniformType eType) noexcept;
        [[nodiscard]] static std::string DescribeUniforms(const ShaderInterface& oInterface);
    };
}
