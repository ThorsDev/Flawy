#pragma once

#include "Flawy/Renderer/Shader/CompiledShader.hpp"
#include "Flawy/Renderer/Shader/SpecializedShaderRequest.hpp"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

namespace Flawy::Renderer
{
    struct ShaderCacheKey
    {
        uint64_t uLow = 0;
        uint64_t uHigh = 0;

        [[nodiscard]] std::string ToHex() const;
        [[nodiscard]] bool operator==(const ShaderCacheKey&) const noexcept = default;

        [[nodiscard]] static ShaderCacheKey FromRequest(const SpecializedShaderRequest& oRequest, std::string_view svCompilerFingerprint);
    };

    struct ShaderCacheStatistics
    {
        uint32_t uDiskHits = 0;
        uint32_t uMisses = 0;
        uint32_t uStores = 0;
    };

    class ShaderCache final
    {
    public:
        explicit ShaderCache(std::optional<std::filesystem::path> optDirectory);
        ~ShaderCache();

        ShaderCache(const ShaderCache&) = delete;
        ShaderCache& operator=(const ShaderCache&) = delete;
        ShaderCache(ShaderCache&&) = delete;
        ShaderCache& operator=(ShaderCache&&) = delete;

        [[nodiscard]] std::optional<CompiledShader> Find(const ShaderCacheKey& oKey);
        void Store(const ShaderCacheKey& oKey, const CompiledShader& oShader);
        [[nodiscard]] const ShaderCacheStatistics& GetStatistics() const noexcept;
        [[nodiscard]] const std::optional<std::filesystem::path>& GetDirectory() const noexcept;
        [[nodiscard]] bool IsEnabled() const noexcept;

        [[nodiscard]] static constexpr uint32_t GetFormatVersion() noexcept
        {
            return 1;
        }

    private:
        struct Impl;
        std::unique_ptr<Impl> m_upImpl;
    };
}
