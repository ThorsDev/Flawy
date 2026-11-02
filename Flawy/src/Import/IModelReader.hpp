#pragma once

#include "Flawy/Import/ImportedScene.hpp"

#include <filesystem>
#include <span>
#include <string_view>

namespace Flawy::Assets
{
    struct AssetImportOptions;
}

namespace Flawy::Import
{
    class IModelReader
    {
    public:
        virtual ~IModelReader() = default;

        [[nodiscard]] virtual std::string_view GetFormatName() const noexcept = 0;
        [[nodiscard]] virtual std::span<const std::string_view> GetExtensions() const noexcept = 0;
        [[nodiscard]] virtual ImportedScene Read(const std::filesystem::path& oPath, const Assets::AssetImportOptions& oOptions) = 0;
    };
}
