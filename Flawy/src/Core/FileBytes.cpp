#include "Flawy/Core/FileBytes.hpp"

#include "Flawy/Core/Hash.hpp"

#include <chrono>
#include <format>
#include <fstream>
#include <ios>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <system_error>

namespace Flawy::Core
{
    namespace
    {
        std::string ToDisplayPath(const std::filesystem::path& oPath)
        {
            const std::u8string sUtf8 = oPath.generic_u8string();
            std::string sText;
            sText.reserve(sUtf8.size());
            for (const char8_t cCharacter : sUtf8)
            {
                sText.push_back(static_cast<char>(cCharacter));
            }
            return sText;
        }

        uint64_t CreateWriteNonce()
        {
            std::random_device oDevice;
            const uint64_t uRandom = (static_cast<uint64_t>(oDevice()) << 32) ^ static_cast<uint64_t>(oDevice());
            const auto uTicks = static_cast<uint64_t>(std::chrono::high_resolution_clock::now().time_since_epoch().count());
            Fnv1aHasher oHasher;
            oHasher.AppendValue(uRandom);
            oHasher.AppendValue(uTicks);
            return oHasher.GetValue();
        }
    }

    std::vector<std::byte> FileBytes::Read(const std::filesystem::path& oPath)
    {
        std::error_code oError;
        if (!std::filesystem::is_regular_file(oPath, oError))
        {
            throw std::runtime_error(std::format("File '{}' does not exist or is not a regular file", ToDisplayPath(oPath)));
        }

        const std::uintmax_t uSize = std::filesystem::file_size(oPath, oError);
        if (oError)
        {
            throw std::runtime_error(std::format("File '{}' size could not be queried: {}", ToDisplayPath(oPath), oError.message()));
        }
        if (uSize > static_cast<std::uintmax_t>(std::numeric_limits<std::streamsize>::max()))
        {
            throw std::runtime_error(std::format("File '{}' is too large to read ({} bytes)", ToDisplayPath(oPath), uSize));
        }

        std::vector<std::byte> vBytes(static_cast<size_t>(uSize));
        std::ifstream oStream(oPath, std::ios::binary);
        oStream.read(reinterpret_cast<char*>(vBytes.data()), static_cast<std::streamsize>(vBytes.size()));
        if (!oStream || oStream.gcount() != static_cast<std::streamsize>(vBytes.size()))
        {
            throw std::runtime_error(std::format("File '{}' could not be read", ToDisplayPath(oPath)));
        }
        return vBytes;
    }

    void FileBytes::WriteAtomically(const std::filesystem::path& oPath, std::span<const std::byte> spanBytes)
    {
        std::error_code oError;
        if (oPath.has_parent_path())
        {
            std::filesystem::create_directories(oPath.parent_path(), oError);
            if (oError)
            {
                throw std::runtime_error(std::format("Directory '{}' could not be created: {}", ToDisplayPath(oPath.parent_path()), oError.message()));
            }
        }

        std::filesystem::path oTemporaryPath = oPath;
        oTemporaryPath += std::format(".{:016x}.tmp", CreateWriteNonce());
        {
            std::ofstream oStream(oTemporaryPath, std::ios::binary | std::ios::trunc);
            oStream.write(reinterpret_cast<const char*>(spanBytes.data()), static_cast<std::streamsize>(spanBytes.size()));
            oStream.close();
            if (!oStream)
            {
                std::filesystem::remove(oTemporaryPath, oError);
                throw std::runtime_error(std::format("File '{}' could not be written", ToDisplayPath(oTemporaryPath)));
            }
        }

        std::filesystem::rename(oTemporaryPath, oPath, oError);
        if (oError)
        {
            const std::string sReason = oError.message();
            std::filesystem::remove(oTemporaryPath, oError);
            throw std::runtime_error(std::format("File '{}' could not be stored: {}", ToDisplayPath(oPath), sReason));
        }
    }
}
