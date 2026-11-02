#include "Flawy/Assets/AssetManager.hpp"

#include "Flawy/Core/FileBytes.hpp"
#include "Flawy/Core/Logger.hpp"
#include "Flawy/Core/StringHash.hpp"

#include <algorithm>
#include <chrono>
#include <cstddef>
#include <format>
#include <functional>
#include <stdexcept>
#include <unordered_map>
#include <vector>

namespace Flawy::Assets
{
    namespace
    {
        constexpr std::string_view kCurrentDirectoryPrefix = "./";
        constexpr std::string_view kCurrentDirectory = ".";
        constexpr std::string_view kParentDirectory = "..";

        template <typename... TArguments> void LogInfo(std::format_string<TArguments...> oFormat, TArguments&&... oArguments)
        {
            if (Core::Logger::IsInitialized())
            {
                Core::Logger::Get().info("{}", std::format(oFormat, std::forward<TArguments>(oArguments)...));
            }
        }

        template <typename... TArguments> void LogDebug(std::format_string<TArguments...> oFormat, TArguments&&... oArguments)
        {
            if (Core::Logger::IsInitialized())
            {
                Core::Logger::Get().debug("{}", std::format(oFormat, std::forward<TArguments>(oArguments)...));
            }
        }

        template <typename... TArguments> void LogWarning(std::format_string<TArguments...> oFormat, TArguments&&... oArguments)
        {
            if (Core::Logger::IsInitialized())
            {
                Core::Logger::Get().warn("{}", std::format(oFormat, std::forward<TArguments>(oArguments)...));
            }
        }

        std::filesystem::path ToUtf8Path(std::string_view svPath)
        {
            std::u8string sUtf8;
            sUtf8.reserve(svPath.size());
            for (const char cCharacter : svPath)
            {
                sUtf8.push_back(static_cast<char8_t>(cCharacter));
            }
            return std::filesystem::path(sUtf8);
        }

        char ToLowerAscii(char cCharacter) noexcept
        {
            return cCharacter >= 'A' && cCharacter <= 'Z' ? static_cast<char>(cCharacter - 'A' + 'a') : cCharacter;
        }

        std::string_view GetFileName(std::string_view svPath) noexcept
        {
            const size_t uSeparator = svPath.find_last_of('/');
            return uSeparator == std::string_view::npos ? svPath : svPath.substr(uSeparator + 1);
        }

        std::string GetStem(std::string_view svPath)
        {
            const std::string_view svFileName = GetFileName(svPath);
            const size_t uDot = svFileName.find_last_of('.');
            return std::string(uDot == std::string_view::npos || uDot == 0 ? svFileName : svFileName.substr(0, uDot));
        }

        std::string GetLowerExtension(std::string_view svPath)
        {
            const std::string_view svFileName = GetFileName(svPath);
            const size_t uDot = svFileName.find_last_of('.');
            if (uDot == std::string_view::npos || uDot == 0 || uDot + 1 == svFileName.size())
            {
                throw std::invalid_argument(std::format("Asset path '{}' has no file extension", svPath));
            }

            std::string sExtension(svFileName.substr(uDot));
            std::ranges::transform(sExtension, sExtension.begin(), ToLowerAscii);
            return sExtension;
        }

        void ValidateRegisteredExtension(std::string_view svExtension, std::string_view svOwner)
        {
            const bool bLowerCase = std::ranges::all_of(svExtension, [](char cCharacter) { return ToLowerAscii(cCharacter) == cCharacter; });
            if (svExtension.size() < 2 || svExtension.front() != '.' || !bLowerCase || svExtension.find('/') != std::string_view::npos)
            {
                throw std::invalid_argument(std::format("{} extension '{}' must be a lower-case extension starting with '.'", svOwner, svExtension));
            }
        }

        class LoadStackGuard final
        {
        public:
            LoadStackGuard(std::vector<std::string>& vStack, std::string sPath)
                : m_vStack(vStack)
            {
                m_vStack.push_back(std::move(sPath));
            }

            ~LoadStackGuard()
            {
                m_vStack.pop_back();
            }

            LoadStackGuard(const LoadStackGuard&) = delete;
            LoadStackGuard& operator=(const LoadStackGuard&) = delete;
            LoadStackGuard(LoadStackGuard&&) = delete;
            LoadStackGuard& operator=(LoadStackGuard&&) = delete;

        private:
            std::vector<std::string>& m_vStack;
        };
    }

    struct AssetManager::Impl
    {
        using Clock = std::chrono::steady_clock;

        template <typename TValue>
        using StringMap = std::unordered_map<std::string, TValue, Core::TransparentStringHash, std::equal_to<>>;

        explicit Impl(AssetManagerSpecification oSpecification)
        {
            if (oSpecification.optAssetRoot)
            {
                optAssetRoot = std::filesystem::absolute(*oSpecification.optAssetRoot).lexically_normal();
            }
        }

        [[nodiscard]] std::string GetTypeName(AssetTypeId oType) const
        {
            const auto itName = mapTypeNames.find(oType.uValue);
            return itName == mapTypeNames.end() ? std::format("0x{:016x}", oType.uValue) : itName->second;
        }

        [[nodiscard]] IAssetLoader& FindLoader(std::string_view svPath) const
        {
            const std::string sExtension = GetLowerExtension(svPath);
            const auto itLoader = mapLoaders.find(sExtension);
            if (itLoader == mapLoaders.end())
            {
                throw std::invalid_argument(std::format("No asset loader registered for extension '{}' (asset '{}')", sExtension, svPath));
            }
            return *itLoader->second;
        }

        [[nodiscard]] IAssetImporter& FindImporter(std::string_view svPath) const
        {
            const std::string sExtension = GetLowerExtension(svPath);
            const auto itImporter = mapImporters.find(sExtension);
            if (itImporter == mapImporters.end())
            {
                throw std::invalid_argument(std::format("No asset importer registered for extension '{}' (source '{}')", sExtension, svPath));
            }
            return *itImporter->second;
        }

        void ThrowIfLoading(const std::string& sPath) const
        {
            const auto itLoading = std::ranges::find(vLoadStack, sPath);
            if (itLoading == vLoadStack.end())
            {
                return;
            }

            std::string sChain;
            for (auto itStep = itLoading; itStep != vLoadStack.end(); ++itStep)
            {
                sChain.append(*itStep).append(" -> ");
            }
            sChain.append(sPath);
            throw std::logic_error(std::format("Asset dependency cycle: {}", sChain));
        }

        void RequireType(std::string_view svPath, AssetTypeId oActual, AssetTypeId oExpected) const
        {
            if (oActual != oExpected)
            {
                throw std::logic_error(std::format("Asset '{}' is of type {} but type {} was requested", svPath, GetTypeName(oActual), GetTypeName(oExpected)));
            }
        }

        std::shared_ptr<Asset> Load(AssetManager& oOwner, std::string_view svRelativePath, std::optional<AssetTypeId> optExpectedType)
        {
            std::string sPath = NormalizePath(svRelativePath);
            const auto itCached = mapCache.find(sPath);
            if (itCached != mapCache.end())
            {
                if (optExpectedType)
                {
                    RequireType(sPath, itCached->second->GetTypeId(), *optExpectedType);
                }
                ++oStatistics.uCacheHits;
                return itCached->second;
            }

            ThrowIfLoading(sPath);
            IAssetLoader& oLoader = FindLoader(sPath);
            if (optExpectedType)
            {
                RequireType(sPath, oLoader.GetTypeId(), *optExpectedType);
            }

            const Clock::time_point tStart = Clock::now();
            const std::filesystem::path oAbsolutePath = oOwner.ResolvePath(sPath);
            const std::vector<std::byte> vBytes = Core::FileBytes::Read(oAbsolutePath);

            std::shared_ptr<Asset> spAsset;
            {
                LoadStackGuard oGuard(vLoadStack, sPath);
                const AssetLoadContext oContext{AssetId::FromPath(sPath), GetStem(sPath), oAbsolutePath, vBytes, oOwner};
                spAsset = oLoader.Load(oContext);
                ValidateLoaded(oContext, oLoader, spAsset.get());
            }

            if (!mapCache.emplace(sPath, spAsset).second)
            {
                throw std::logic_error(std::format("Asset '{}' was registered while it was being loaded", sPath));
            }
            ++oStatistics.uLoads;
            const std::chrono::duration<double, std::milli> tElapsed = Clock::now() - tStart;
            LogInfo("Loaded asset '{}' ({}) in {:.2f} ms", sPath, oLoader.GetTypeName(), tElapsed.count());
            return spAsset;
        }

        uint32_t ReleaseUnreferenced()
        {
            uint32_t uRemoved = 0;
            bool bRemovedAny = true;
            while (bRemovedAny)
            {
                const size_t uErased = std::erase_if(mapCache, [](const auto& oEntry) { return oEntry.second.use_count() == 1; });
                bRemovedAny = uErased != 0;
                uRemoved += static_cast<uint32_t>(uErased);
            }
            return uRemoved;
        }

        void ValidateLoaded(const AssetLoadContext& oContext, const IAssetLoader& oLoader, const Asset* pAsset) const
        {
            if (pAsset == nullptr)
            {
                throw std::logic_error(std::format("{} loader returned no asset for '{}'", oLoader.GetTypeName(), oContext.oId.GetPath()));
            }
            RequireType(oContext.oId.GetPath(), pAsset->GetTypeId(), oLoader.GetTypeId());
            if (!(pAsset->GetId() == oContext.oId))
            {
                throw std::logic_error(std::format("{} loader returned asset id '{}' for '{}'", oLoader.GetTypeName(), pAsset->GetId().GetPath(), oContext.oId.GetPath()));
            }
        }

        std::optional<std::filesystem::path> optAssetRoot;
        std::vector<std::unique_ptr<IAssetLoader>> vLoaders;
        std::vector<std::unique_ptr<IAssetImporter>> vImporters;
        StringMap<IAssetLoader*> mapLoaders;
        StringMap<IAssetImporter*> mapImporters;
        std::unordered_map<uint64_t, std::string> mapTypeNames;
        StringMap<std::shared_ptr<Asset>> mapCache;
        std::vector<std::string> vLoadStack;
        AssetManagerStatistics oStatistics;
    };

    AssetManager::AssetManager(AssetManagerSpecification oSpecification)
        : m_upImpl(std::make_unique<Impl>(std::move(oSpecification)))
    {
    }

    AssetManager::~AssetManager()
    {
        m_upImpl->ReleaseUnreferenced();
        for (const auto& [sPath, spAsset] : m_upImpl->mapCache)
        {
            if (spAsset.use_count() > 1)
            {
                LogWarning("Asset '{}' is still referenced by {} handle(s) when the asset manager is destroyed", sPath, spAsset.use_count() - 1);
            }
        }
        m_upImpl->mapCache.clear();
    }

    void AssetManager::RegisterLoader(std::unique_ptr<IAssetLoader> upLoader)
    {
        if (!upLoader)
        {
            throw std::invalid_argument("Asset loader must not be null");
        }

        const std::span<const std::string_view> spanExtensions = upLoader->GetExtensions();
        if (spanExtensions.empty())
        {
            throw std::invalid_argument(std::format("{} loader declares no extensions", upLoader->GetTypeName()));
        }
        for (const std::string_view svExtension : spanExtensions)
        {
            ValidateRegisteredExtension(svExtension, "Asset loader");
            if (m_upImpl->mapLoaders.contains(svExtension))
            {
                throw std::invalid_argument(std::format("An asset loader is already registered for extension '{}'", svExtension));
            }
        }

        for (const std::string_view svExtension : spanExtensions)
        {
            m_upImpl->mapLoaders.emplace(std::string(svExtension), upLoader.get());
        }
        m_upImpl->mapTypeNames.try_emplace(upLoader->GetTypeId().uValue, std::string(upLoader->GetTypeName()));
        m_upImpl->vLoaders.push_back(std::move(upLoader));
    }

    void AssetManager::RegisterImporter(std::unique_ptr<IAssetImporter> upImporter)
    {
        if (!upImporter)
        {
            throw std::invalid_argument("Asset importer must not be null");
        }

        const std::span<const std::string_view> spanExtensions = upImporter->GetExtensions();
        if (spanExtensions.empty())
        {
            throw std::invalid_argument("Asset importer declares no extensions");
        }
        for (const std::string_view svExtension : spanExtensions)
        {
            ValidateRegisteredExtension(svExtension, "Asset importer");
            if (m_upImpl->mapImporters.contains(svExtension))
            {
                throw std::invalid_argument(std::format("An asset importer is already registered for extension '{}'", svExtension));
            }
        }

        for (const std::string_view svExtension : spanExtensions)
        {
            m_upImpl->mapImporters.emplace(std::string(svExtension), upImporter.get());
        }
        m_upImpl->vImporters.push_back(std::move(upImporter));
    }

    std::shared_ptr<Asset> AssetManager::LoadUntyped(std::string_view svRelativePath, AssetTypeId oExpectedType)
    {
        return m_upImpl->Load(*this, svRelativePath, oExpectedType);
    }

    std::shared_ptr<Asset> AssetManager::LoadUntyped(std::string_view svRelativePath)
    {
        return m_upImpl->Load(*this, svRelativePath, std::nullopt);
    }

    AssetImportResult AssetManager::Import(std::string_view svRelativeSourcePath, const AssetImportOptions& oOptions)
    {
        const std::string sPath = NormalizePath(svRelativeSourcePath);
        IAssetImporter& oImporter = m_upImpl->FindImporter(sPath);
        const std::filesystem::path oSourcePath = ResolvePath(sPath);
        if (!std::filesystem::is_regular_file(oSourcePath))
        {
            throw std::runtime_error(std::format("Import source '{}' does not exist or is not a file", sPath));
        }

        const AssetImportContext oContext{oSourcePath, sPath, GetStem(sPath), oOptions, *this};
        AssetImportResult oResult = oImporter.Import(oContext);
        for (const std::shared_ptr<Asset>& spAsset : oResult.vAssets)
        {
            LogInfo("Imported asset '{}' ({}) from '{}'", spAsset->GetId().GetPath(), m_upImpl->GetTypeName(spAsset->GetTypeId()), sPath);
        }
        return oResult;
    }

    bool AssetManager::IsLoaded(std::string_view svRelativePath) const noexcept
    {
        try
        {
            return m_upImpl->mapCache.contains(NormalizePath(svRelativePath));
        }
        catch (const std::exception&)
        {
            return false;
        }
    }

    bool AssetManager::Unload(std::string_view svRelativePath)
    {
        const std::string sPath = NormalizePath(svRelativePath);
        if (m_upImpl->mapCache.erase(sPath) == 0)
        {
            return false;
        }

        ++m_upImpl->oStatistics.uUnloads;
        LogDebug("Unloaded asset '{}'", sPath);
        return true;
    }

    uint32_t AssetManager::UnloadUnused()
    {
        const uint32_t uRemoved = m_upImpl->ReleaseUnreferenced();
        m_upImpl->oStatistics.uUnloads += uRemoved;
        if (uRemoved != 0)
        {
            LogDebug("Unloaded {} unused asset(s)", uRemoved);
        }
        return uRemoved;
    }

    uint32_t AssetManager::GetLoadedCount() const noexcept
    {
        return static_cast<uint32_t>(m_upImpl->mapCache.size());
    }

    const AssetManagerStatistics& AssetManager::GetStatistics() const noexcept
    {
        return m_upImpl->oStatistics;
    }

    const std::optional<std::filesystem::path>& AssetManager::GetAssetRoot() const noexcept
    {
        return m_upImpl->optAssetRoot;
    }

    std::filesystem::path AssetManager::ResolvePath(std::string_view svRelativePath) const
    {
        const std::string sPath = NormalizePath(svRelativePath);
        if (!m_upImpl->optAssetRoot)
        {
            throw std::logic_error(std::format("Asset '{}' cannot be resolved because the asset manager has no asset root", sPath));
        }
        return *m_upImpl->optAssetRoot / ToUtf8Path(sPath);
    }

    std::string AssetManager::NormalizePath(std::string_view svRelativePath)
    {
        std::string sPath(svRelativePath);
        std::ranges::replace(sPath, '\\', '/');
        while (std::string_view(sPath).starts_with(kCurrentDirectoryPrefix))
        {
            sPath.erase(0, kCurrentDirectoryPrefix.size());
        }

        if (sPath.empty())
        {
            throw std::invalid_argument(std::format("Asset path '{}' must not be empty", svRelativePath));
        }
        if (sPath.front() == '/')
        {
            throw std::invalid_argument(std::format("Asset path '{}' must be relative to the asset root", svRelativePath));
        }

        std::string_view svRemaining = sPath;
        bool bFirstSegment = true;
        while (true)
        {
            const size_t uSeparator = svRemaining.find('/');
            const std::string_view svSegment = svRemaining.substr(0, uSeparator);
            if (svSegment.empty() || svSegment == kCurrentDirectory || svSegment == kParentDirectory)
            {
                throw std::invalid_argument(std::format("Asset path '{}' must be a relative path without empty, '.' or '..' segments", svRelativePath));
            }
            if (bFirstSegment && svSegment.find(':') != std::string_view::npos)
            {
                throw std::invalid_argument(std::format("Asset path '{}' must be relative to the asset root", svRelativePath));
            }
            if (uSeparator == std::string_view::npos)
            {
                break;
            }
            svRemaining.remove_prefix(uSeparator + 1);
            bFirstSegment = false;
        }
        return sPath;
    }

    void AssetManager::InsertUntyped(std::string_view svRelativePath, std::shared_ptr<Asset> spAsset, AssetTypeId oExpectedType)
    {
        if (!spAsset)
        {
            throw std::invalid_argument(std::format("Cannot insert a null asset under '{}'", svRelativePath));
        }

        std::string sPath = NormalizePath(svRelativePath);
        if (spAsset->GetTypeId() != oExpectedType)
        {
            throw std::invalid_argument(std::format("Asset inserted under '{}' is of type {} (0x{:016x}) but the handle expects type {} (0x{:016x})",
                                                    sPath,
                                                    m_upImpl->GetTypeName(spAsset->GetTypeId()),
                                                    spAsset->GetTypeId().uValue,
                                                    m_upImpl->GetTypeName(oExpectedType),
                                                    oExpectedType.uValue));
        }
        if (!(spAsset->GetId() == AssetId::FromPath(sPath)))
        {
            throw std::invalid_argument(std::format("Asset id '{}' does not match the insert path '{}'", spAsset->GetId().GetPath(), sPath));
        }
        if (m_upImpl->mapCache.contains(sPath))
        {
            throw std::invalid_argument(std::format("An asset is already registered under '{}'", sPath));
        }

        LogDebug("Inserted asset '{}' ({})", sPath, m_upImpl->GetTypeName(spAsset->GetTypeId()));
        m_upImpl->mapCache.emplace(std::move(sPath), std::move(spAsset));
    }
}
