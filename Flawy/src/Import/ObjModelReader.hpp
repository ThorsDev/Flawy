#pragma once

#include "Import/IModelReader.hpp"

namespace Flawy::Import
{
    class ObjModelReader final : public IModelReader
    {
    public:
        [[nodiscard]] std::string_view GetFormatName() const noexcept override;
        [[nodiscard]] std::span<const std::string_view> GetExtensions() const noexcept override;
        [[nodiscard]] ImportedScene Read(const std::filesystem::path& oPath, const Assets::AssetImportOptions& oOptions) override;
    };
}
