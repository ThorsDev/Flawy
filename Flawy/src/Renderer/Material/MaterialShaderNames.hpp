#pragma once

#include <cstdint>
#include <format>
#include <string>
#include <string_view>

namespace Flawy::Renderer
{
    class MaterialShaderNames final
    {
    public:
        static constexpr std::string_view kMaterialParamsUniform = "u_materialParams";
        static constexpr std::string_view kMaterialTexturePrefix = "s_materialTexture";
        static constexpr std::string_view kViewParamsUniform = "u_viewParams";
        static constexpr std::string_view kLightParamsUniform = "u_lightParams";
        static constexpr uint16_t kViewParamsCount = 2;
        static constexpr uint16_t kLightParamsCount = 12;
        static constexpr uint16_t kMaxPointLights = 4;

        MaterialShaderNames() = delete;

        [[nodiscard]] static std::string GetTextureSamplerName(uint8_t uStage)
        {
            return std::format("{}{}", kMaterialTexturePrefix, uStage);
        }
    };
}
