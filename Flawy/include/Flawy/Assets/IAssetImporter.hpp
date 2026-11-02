#pragma once

#include <filesystem>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace Flawy::Assets
{
    class Asset;
    class AssetManager;

    struct AssetImportOptions
    {
        bool bSaveToAssetRoot = false;
        std::string sOutputDirectory;
        bool bImportMeshes = true;
        bool bImportSkeletons = true;
        bool bImportAnimations = true;
    };

    struct AssetImportResult
    {
        std::vector<std::shared_ptr<Asset>> vAssets;
        std::vector<std::string> vSavedPaths;
    };

    struct AssetImportContext
    {
        std::filesystem::path oSourcePath;
        std::string sSourceAssetPath;
        std::string sSourceName;
        const AssetImportOptions& oOptions;
        AssetManager& oManager;
    };

    class IAssetImporter
    {
    public:
        virtual ~IAssetImporter() = default;

        [[nodiscard]] virtual std::span<const std::string_view> GetExtensions() const noexcept = 0;
        [[nodiscard]] virtual AssetImportResult Import(const AssetImportContext& oContext) = 0;
    };
}
