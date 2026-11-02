#pragma once

#include "Flawy/Renderer/Shader/IShaderSourceProvider.hpp"

#include <filesystem>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace Flawy::Renderer
{
    class FileShaderSourceProvider final : public IShaderSourceProvider
    {
    public:
        explicit FileShaderSourceProvider(std::vector<std::filesystem::path> vSearchRoots);

        [[nodiscard]] std::optional<ShaderSource> Load(std::string_view svPath) const override;
        [[nodiscard]] std::span<const std::filesystem::path> GetSearchRoots() const noexcept;

    private:
        std::vector<std::filesystem::path> m_vSearchRoots;
    };
}
