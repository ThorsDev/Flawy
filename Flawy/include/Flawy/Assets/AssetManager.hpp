#pragma once

#include "Flawy/Assets/Asset.hpp"
#include "Flawy/Assets/AssetHandle.hpp"
#include "Flawy/Assets/AssetTypeId.hpp"
#include "Flawy/Assets/IAssetImporter.hpp"
#include "Flawy/Assets/IAssetLoader.hpp"

#include <concepts>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace Flawy::Assets
{
    struct AssetManagerSpecification
    {
        std::optional<std::filesystem::path> optAssetRoot;
    };

    struct AssetManagerStatistics
    {
        uint64_t uLoads = 0;
        uint64_t uCacheHits = 0;
        uint64_t uUnloads = 0;
    };

    class AssetManager final
    {
    public:
        explicit AssetManager(AssetManagerSpecification oSpecification);
        ~AssetManager();

        AssetManager(const AssetManager&) = delete;
        AssetManager& operator=(const AssetManager&) = delete;
        AssetManager(AssetManager&&) = delete;
        AssetManager& operator=(AssetManager&&) = delete;

        void RegisterLoader(std::unique_ptr<IAssetLoader> upLoader);
        void RegisterImporter(std::unique_ptr<IAssetImporter> upImporter);

        template <std::derived_from<Asset> TAsset>
        [[nodiscard]] AssetHandle<TAsset> Load(std::string_view svRelativePath)
        {
            std::shared_ptr<Asset> spAsset = LoadUntyped(svRelativePath, TAsset::kTypeId);
            return AssetHandle<TAsset>(std::static_pointer_cast<TAsset>(std::move(spAsset)));
        }

        [[nodiscard]] std::shared_ptr<Asset> LoadUntyped(std::string_view svRelativePath, AssetTypeId oExpectedType);
        [[nodiscard]] std::shared_ptr<Asset> LoadUntyped(std::string_view svRelativePath);

        template <std::derived_from<Asset> TAsset>
        AssetHandle<TAsset> Insert(std::string_view svRelativePath, std::shared_ptr<TAsset> spAsset)
        {
            InsertUntyped(svRelativePath, spAsset, TAsset::kTypeId);
            return AssetHandle<TAsset>(std::move(spAsset));
        }

        [[nodiscard]] AssetImportResult Import(std::string_view svRelativeSourcePath, const AssetImportOptions& oOptions);

        [[nodiscard]] bool IsLoaded(std::string_view svRelativePath) const noexcept;
        bool Unload(std::string_view svRelativePath);
        uint32_t UnloadUnused();
        [[nodiscard]] uint32_t GetLoadedCount() const noexcept;
        [[nodiscard]] const AssetManagerStatistics& GetStatistics() const noexcept;
        [[nodiscard]] const std::optional<std::filesystem::path>& GetAssetRoot() const noexcept;
        [[nodiscard]] std::filesystem::path ResolvePath(std::string_view svRelativePath) const;
        [[nodiscard]] static std::string NormalizePath(std::string_view svRelativePath);

    private:
        void InsertUntyped(std::string_view svRelativePath, std::shared_ptr<Asset> spAsset, AssetTypeId oExpectedType);

        struct Impl;
        std::unique_ptr<Impl> m_upImpl;
    };
}
