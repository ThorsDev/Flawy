#pragma once

#include "Flawy/Renderer/Shader/CompiledShader.hpp"
#include "Flawy/Renderer/Shader/ShaderCache.hpp"

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <vector>

namespace Flawy::Renderer
{
    struct ShaderCacheReadResult
    {
        std::optional<CompiledShader> optShader;
        std::string sError;
    };

    class ShaderCacheSerializer final
    {
    public:
        [[nodiscard]] std::vector<std::byte> Serialize(const ShaderCacheKey& oKey, const CompiledShader& oShader) const;
        [[nodiscard]] ShaderCacheReadResult Deserialize(const ShaderCacheKey& oKey, std::span<const std::byte> spanBytes) const;
    };
}
