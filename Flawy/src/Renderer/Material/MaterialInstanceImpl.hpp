#pragma once

#include "Flawy/Renderer/Material/Material.hpp"
#include "Flawy/Renderer/Material/MaterialCompiler.hpp"
#include "Flawy/Renderer/Material/MaterialInstance.hpp"
#include "Flawy/Renderer/SamplerState.hpp"
#include "Flawy/Renderer/ShaderParameters.hpp"

#include <glm/vec4.hpp>

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace Flawy::Renderer
{
    class Texture;

    struct MaterialInstance::Impl
    {
        struct TextureBinding
        {
            const Texture* pTexture = nullptr;
            std::optional<SamplerState> optSampler;
        };

        explicit Impl(Material& oMaterial);

        [[nodiscard]] const ShaderParameters& AcquireParameters(const MaterialPermutation& oPermutation) const;
        void MarkDirty() noexcept;

        Material* pMaterial;
        std::vector<glm::vec4> vSlots;
        std::vector<TextureBinding> vTextures;
        std::vector<std::string> vTextureSamplerNames;
        std::vector<MaterialStaticSwitchValue> vStaticSwitches;
        uint32_t uStaticSwitchSet = 0;
        mutable std::vector<std::unique_ptr<ShaderParameters>> vPermutationParameters;
        mutable std::vector<bool> vSynced;

    private:
        void Synchronize(ShaderParameters& oParameters) const;
    };
}
