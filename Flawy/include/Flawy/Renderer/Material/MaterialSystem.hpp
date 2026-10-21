#pragma once

#include "Flawy/Renderer/RendererBackend.hpp"

#include <filesystem>
#include <memory>
#include <optional>

namespace Flawy::Renderer
{
    class RenderDevice;
    class ShaderCache;
    class ShaderCompiler;
    class Texture;

    struct MaterialSystemSpecification
    {
        std::optional<std::filesystem::path> optShaderCacheDirectory;
    };

    class MaterialSystem final
    {
    public:
        explicit MaterialSystem(RenderDevice& oDevice, const MaterialSystemSpecification& oSpecification = {});
        ~MaterialSystem();

        MaterialSystem(const MaterialSystem&) = delete;
        MaterialSystem& operator=(const MaterialSystem&) = delete;
        MaterialSystem(MaterialSystem&&) = delete;
        MaterialSystem& operator=(MaterialSystem&&) = delete;

        [[nodiscard]] ShaderCompiler& GetCompiler() noexcept;
        [[nodiscard]] ShaderCache& GetShaderCache() noexcept;
        [[nodiscard]] const Texture& GetWhiteTexture() const noexcept;
        [[nodiscard]] const Texture& GetFlatNormalTexture() const noexcept;
        [[nodiscard]] RendererBackend GetBackend() const noexcept;

    private:
        struct Impl;
        std::unique_ptr<Impl> m_upImpl;
    };
}
