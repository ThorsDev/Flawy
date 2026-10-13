#pragma once

#include "Flawy/Renderer/Shader/ShaderStage.hpp"
#include "Renderer/Shader/BgfxShaderInterface.hpp"

#include <cstddef>
#include <span>
#include <vector>

namespace Flawy::Renderer
{
    class BgfxShaderBinaryWriter final
    {
    public:
        [[nodiscard]] std::vector<std::byte> Write(ShaderStage eStage, const BgfxShaderInterface& oInterface, std::span<const std::byte> spanCode) const;
    };
}
