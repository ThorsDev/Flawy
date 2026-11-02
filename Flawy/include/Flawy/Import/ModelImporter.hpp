#pragma once

#include "Flawy/Assets/IAssetImporter.hpp"

#include <memory>
#include <span>
#include <string_view>

namespace Flawy::Import
{
    class IModelReader;

    class ModelImporter final : public Assets::IAssetImporter
    {
    public:
        explicit ModelImporter(std::unique_ptr<IModelReader> upReader);
        ~ModelImporter() override;

        ModelImporter(const ModelImporter&) = delete;
        ModelImporter& operator=(const ModelImporter&) = delete;
        ModelImporter(ModelImporter&&) = delete;
        ModelImporter& operator=(ModelImporter&&) = delete;

        [[nodiscard]] std::span<const std::string_view> GetExtensions() const noexcept override;
        [[nodiscard]] Assets::AssetImportResult Import(const Assets::AssetImportContext& oContext) override;

    private:
        std::unique_ptr<IModelReader> m_upReader;
    };
}
