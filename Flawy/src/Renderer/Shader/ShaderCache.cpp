#include "Flawy/Renderer/Shader/ShaderCache.hpp"

#include "Flawy/Core/Hash.hpp"
#include "Flawy/Core/Logger.hpp"
#include "Renderer/Shader/ShaderCacheSerializer.hpp"

#include <chrono>
#include <format>
#include <fstream>
#include <ios>
#include <random>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

namespace Flawy::Renderer
{
    namespace
    {
        constexpr std::string_view kFileExtension = ".flsc";
        constexpr std::string_view kTemporaryExtension = ".tmp";
        constexpr std::uintmax_t kMaxEntryFileSize = 64ull * 1024ull * 1024ull;

        enum class ShaderCacheRequestKind : uint8_t
        {
            Source = 0,
            Specialized = 1
        };

        class ShaderCacheKeyBuilder final
        {
        public:
            explicit ShaderCacheKeyBuilder(uint64_t uSeed) noexcept
                : m_oHasher(uSeed)
            {
            }

            void AppendRequest(const SpecializedShaderRequest& oRequest, std::string_view svCompilerFingerprint) noexcept
            {
                m_oHasher.AppendValue(ShaderCache::GetFormatVersion());
                m_oHasher.AppendValue(ShaderCacheRequestKind::Specialized);
                m_oHasher.Append(svCompilerFingerprint);
                m_oHasher.AppendValue(oRequest.eStage);
                m_oHasher.Append(oRequest.sEntryPoint);
                AppendModule(oRequest.oPassModule);
                m_oHasher.AppendValue(static_cast<uint64_t>(oRequest.vModules.size()));
                for (const ShaderModuleSource& oModule : oRequest.vModules)
                {
                    AppendModule(oModule);
                }
                m_oHasher.AppendValue(static_cast<uint64_t>(oRequest.vTypeArguments.size()));
                for (const std::string& sTypeArgument : oRequest.vTypeArguments)
                {
                    m_oHasher.Append(sTypeArgument);
                }
            }

            void AppendSource(const ShaderSource& oSource, ShaderStage eStage, std::string_view svEntryPoint, std::string_view svCompilerFingerprint) noexcept
            {
                m_oHasher.AppendValue(ShaderCache::GetFormatVersion());
                m_oHasher.AppendValue(ShaderCacheRequestKind::Source);
                m_oHasher.Append(svCompilerFingerprint);
                m_oHasher.AppendValue(eStage);
                m_oHasher.Append(svEntryPoint);
                m_oHasher.Append(oSource.sName);
                m_oHasher.Append(oSource.sPath);
                m_oHasher.Append(oSource.sCode);
                m_oHasher.AppendValue(static_cast<uint64_t>(oSource.vDefines.size()));
                for (const ShaderDefine& oDefine : oSource.vDefines)
                {
                    m_oHasher.Append(oDefine.sName);
                    m_oHasher.Append(oDefine.sValue);
                }
            }

            [[nodiscard]] uint64_t GetValue() const noexcept
            {
                return m_oHasher.GetValue();
            }

        private:
            void AppendModule(const ShaderModuleSource& oModule) noexcept
            {
                m_oHasher.Append(oModule.sName);
                m_oHasher.Append(oModule.sPath);
                m_oHasher.Append(oModule.sCode);
            }

            Core::Fnv1aHasher m_oHasher;
        };

        template <typename... TArguments> void LogWarning(std::format_string<TArguments...> oFormat, TArguments&&... oArguments)
        {
            if (Core::Logger::IsInitialized())
            {
                Core::Logger::Get().warn("{}", std::format(oFormat, std::forward<TArguments>(oArguments)...));
            }
        }

        template <typename... TArguments> void LogDebug(std::format_string<TArguments...> oFormat, TArguments&&... oArguments)
        {
            if (Core::Logger::IsInitialized())
            {
                Core::Logger::Get().debug("{}", std::format(oFormat, std::forward<TArguments>(oArguments)...));
            }
        }

        std::string ToDisplayPath(const std::filesystem::path& oPath)
        {
            const std::u8string sPath = oPath.generic_u8string();
            return std::string(reinterpret_cast<const char*>(sPath.data()), sPath.size());
        }

        uint64_t CreateWriterNonce()
        {
            std::random_device oDevice;
            const uint64_t uRandom = (static_cast<uint64_t>(oDevice()) << 32) ^ static_cast<uint64_t>(oDevice());
            const auto uTicks = static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
            Core::Fnv1aHasher oHasher;
            oHasher.AppendValue(uRandom);
            oHasher.AppendValue(uTicks);
            return oHasher.GetValue();
        }
    }

    std::string ShaderCacheKey::ToHex() const
    {
        return std::format("{:016x}{:016x}", uHigh, uLow);
    }

    ShaderCacheKey ShaderCacheKey::FromRequest(const SpecializedShaderRequest& oRequest, std::string_view svCompilerFingerprint)
    {
        ShaderCacheKeyBuilder oLow(Core::Fnv1aHasher::kDefaultSeed);
        ShaderCacheKeyBuilder oHigh(Core::Fnv1aHasher::kAlternateSeed);
        oLow.AppendRequest(oRequest, svCompilerFingerprint);
        oHigh.AppendRequest(oRequest, svCompilerFingerprint);
        return {oLow.GetValue(), oHigh.GetValue()};
    }

    ShaderCacheKey ShaderCacheKey::FromSource(const ShaderSource& oSource, ShaderStage eStage, std::string_view svEntryPoint, std::string_view svCompilerFingerprint)
    {
        ShaderCacheKeyBuilder oLow(Core::Fnv1aHasher::kDefaultSeed);
        ShaderCacheKeyBuilder oHigh(Core::Fnv1aHasher::kAlternateSeed);
        oLow.AppendSource(oSource, eStage, svEntryPoint, svCompilerFingerprint);
        oHigh.AppendSource(oSource, eStage, svEntryPoint, svCompilerFingerprint);
        return {oLow.GetValue(), oHigh.GetValue()};
    }

    struct ShaderCache::Impl
    {
        explicit Impl(std::optional<std::filesystem::path> optCacheDirectory)
            : optDirectory(std::move(optCacheDirectory))
            , uWriterNonce(optDirectory ? CreateWriterNonce() : 0)
        {
        }

        [[nodiscard]] std::filesystem::path GetEntryPath(const ShaderCacheKey& oKey) const
        {
            return *optDirectory / (oKey.ToHex() + std::string(kFileExtension));
        }

        [[nodiscard]] std::filesystem::path MakeTemporaryPath(const ShaderCacheKey& oKey)
        {
            ++uWriteCounter;
            return *optDirectory / std::format("{}{}.{:016x}.{}{}", oKey.ToHex(), kFileExtension, uWriterNonce, uWriteCounter, kTemporaryExtension);
        }

        [[nodiscard]] std::optional<CompiledShader> LoadFromDisk(const ShaderCacheKey& oKey) const
        {
            const std::filesystem::path oPath = GetEntryPath(oKey);
            std::error_code oError;
            if (!std::filesystem::is_regular_file(oPath, oError))
            {
                return std::nullopt;
            }

            ShaderCacheReadResult oResult = ReadEntry(oKey, oPath);
            if (oResult.optShader)
            {
                return std::move(oResult.optShader);
            }

            std::filesystem::remove(oPath, oError);
            LogWarning("Shader cache entry {} is invalid ({}); it was {} and the shader will be recompiled",
                       ToDisplayPath(oPath),
                       oResult.sError,
                       oError ? std::format("not deleted: {}", oError.message()) : std::string("deleted"));
            return std::nullopt;
        }

        [[nodiscard]] static ShaderCacheReadResult ReadEntry(const ShaderCacheKey& oKey, const std::filesystem::path& oPath)
        {
            std::error_code oError;
            const std::uintmax_t uSize = std::filesystem::file_size(oPath, oError);
            if (oError || uSize > kMaxEntryFileSize)
            {
                return {std::nullopt, oError ? oError.message() : std::format("file is {} bytes, larger than the {} byte limit", uSize, kMaxEntryFileSize)};
            }

            std::vector<std::byte> vBytes(static_cast<size_t>(uSize));
            std::ifstream oStream(oPath, std::ios::binary);
            oStream.read(reinterpret_cast<char*>(vBytes.data()), static_cast<std::streamsize>(vBytes.size()));
            if (!oStream || oStream.gcount() != static_cast<std::streamsize>(vBytes.size()))
            {
                return {std::nullopt, "the file could not be read"};
            }
            return ShaderCacheSerializer().Deserialize(oKey, vBytes);
        }

        void WriteToDisk(const ShaderCacheKey& oKey, const CompiledShader& oShader)
        {
            std::error_code oError;
            std::filesystem::create_directories(*optDirectory, oError);
            if (oError)
            {
                LogWarning("Shader cache directory {} could not be created: {}", ToDisplayPath(*optDirectory), oError.message());
                return;
            }

            const std::vector<std::byte> vBytes = ShaderCacheSerializer().Serialize(oKey, oShader);
            const std::filesystem::path oPath = GetEntryPath(oKey);
            const std::filesystem::path oTemporaryPath = MakeTemporaryPath(oKey);
            {
                std::ofstream oStream(oTemporaryPath, std::ios::binary | std::ios::trunc);
                oStream.write(reinterpret_cast<const char*>(vBytes.data()), static_cast<std::streamsize>(vBytes.size()));
                oStream.close();
                if (!oStream)
                {
                    std::filesystem::remove(oTemporaryPath, oError);
                    LogWarning("Shader cache entry {} could not be written", ToDisplayPath(oTemporaryPath));
                    return;
                }
            }

            std::filesystem::rename(oTemporaryPath, oPath, oError);
            if (oError)
            {
                const std::string sReason = oError.message();
                std::filesystem::remove(oTemporaryPath, oError);
                LogWarning("Shader cache entry {} could not be stored: {}", ToDisplayPath(oPath), sReason);
            }
        }

        std::optional<std::filesystem::path> optDirectory;
        uint64_t uWriterNonce;
        uint64_t uWriteCounter = 0;
        ShaderCacheStatistics oStatistics;
    };

    ShaderCache::ShaderCache(std::optional<std::filesystem::path> optDirectory)
        : m_upImpl(std::make_unique<Impl>(std::move(optDirectory)))
    {
    }

    ShaderCache::~ShaderCache() = default;

    std::optional<CompiledShader> ShaderCache::Find(const ShaderCacheKey& oKey)
    {
        if (!IsEnabled())
        {
            return std::nullopt;
        }

        std::optional<CompiledShader> optShader = m_upImpl->LoadFromDisk(oKey);
        if (optShader)
        {
            ++m_upImpl->oStatistics.uDiskHits;
            LogDebug("Shader cache hit {} ({})", oKey.ToHex(), optShader->sName);
            return optShader;
        }

        ++m_upImpl->oStatistics.uMisses;
        LogDebug("Shader cache miss {}", oKey.ToHex());
        return std::nullopt;
    }

    void ShaderCache::Store(const ShaderCacheKey& oKey, const CompiledShader& oShader)
    {
        if (!IsEnabled())
        {
            return;
        }

        ++m_upImpl->oStatistics.uStores;
        m_upImpl->WriteToDisk(oKey, oShader);
    }

    const ShaderCacheStatistics& ShaderCache::GetStatistics() const noexcept
    {
        return m_upImpl->oStatistics;
    }

    const std::optional<std::filesystem::path>& ShaderCache::GetDirectory() const noexcept
    {
        return m_upImpl->optDirectory;
    }

    bool ShaderCache::IsEnabled() const noexcept
    {
        return m_upImpl->optDirectory.has_value();
    }
}
