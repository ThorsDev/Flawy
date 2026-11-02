#include "Flawy/Assets/AssetFile.hpp"

#include "Flawy/Core/BinaryFormatError.hpp"
#include "Flawy/Core/BinaryReader.hpp"
#include "Flawy/Core/BinaryWriter.hpp"
#include "Flawy/Core/FileBytes.hpp"
#include "Flawy/Core/Hash.hpp"

#include <algorithm>
#include <format>

namespace Flawy::Assets
{
    namespace
    {
        uint64_t ComputeChecksum(std::span<const std::byte> spanBytes) noexcept
        {
            Core::Fnv1aHasher oHasher;
            oHasher.Append(spanBytes);
            return oHasher.GetValue();
        }
    }

    std::vector<std::byte> AssetFile::Encode(AssetTypeId oType, uint32_t uFormatVersion, std::span<const std::byte> spanPayload)
    {
        Core::BinaryWriter oWriter;
        oWriter.WriteBytes(std::as_bytes(std::span<const char>(kMagic.data(), kMagic.size())));
        oWriter.WriteU32(kHeaderVersion);
        oWriter.WriteU64(oType.uValue);
        oWriter.WriteU32(uFormatVersion);
        oWriter.WriteU64(static_cast<uint64_t>(spanPayload.size()));
        oWriter.WriteBytes(spanPayload);
        oWriter.WriteU64(ComputeChecksum(oWriter.GetBytes()));
        return oWriter.TakeBytes();
    }

    AssetFile::Contents AssetFile::Decode(std::span<const std::byte> spanFile, AssetTypeId oExpectedType)
    {
        Core::BinaryReader oReader(spanFile);
        const std::span<const std::byte> spanMagic = oReader.ReadBytes(kMagic.size());
        if (!std::ranges::equal(spanMagic, std::as_bytes(std::span<const char>(kMagic.data(), kMagic.size()))))
        {
            throw Core::BinaryFormatError(std::format("bad magic, expected '{}'", kMagic), 0);
        }

        const size_t uHeaderVersionOffset = oReader.GetOffset();
        const uint32_t uHeaderVersion = oReader.ReadU32();
        if (uHeaderVersion != kHeaderVersion)
        {
            throw Core::BinaryFormatError(std::format("unsupported asset header version {}, expected {}", uHeaderVersion, kHeaderVersion), uHeaderVersionOffset);
        }

        const size_t uTypeOffset = oReader.GetOffset();
        const uint64_t uType = oReader.ReadU64();
        if (uType != oExpectedType.uValue)
        {
            throw Core::BinaryFormatError(std::format("asset type id 0x{:016x} does not match the expected type id 0x{:016x}", uType, oExpectedType.uValue), uTypeOffset);
        }

        Contents oContents;
        oContents.uFormatVersion = oReader.ReadU32();

        const size_t uSizeOffset = oReader.GetOffset();
        const uint64_t uPayloadSize = oReader.ReadU64();
        if (oReader.GetRemaining() < kChecksumSize || uPayloadSize != oReader.GetRemaining() - kChecksumSize)
        {
            throw Core::BinaryFormatError(std::format("payload size {} does not match the file size {}", uPayloadSize, spanFile.size()), uSizeOffset);
        }

        oContents.spanPayload = oReader.ReadBytes(static_cast<size_t>(uPayloadSize));
        const size_t uChecksumOffset = oReader.GetOffset();
        const uint64_t uExpectedChecksum = ComputeChecksum(spanFile.first(uChecksumOffset));
        const uint64_t uStoredChecksum = oReader.ReadU64();
        if (uStoredChecksum != uExpectedChecksum)
        {
            throw Core::BinaryFormatError(std::format("checksum mismatch, stored 0x{:016x} but computed 0x{:016x}", uStoredChecksum, uExpectedChecksum), uChecksumOffset);
        }
        return oContents;
    }

    void AssetFile::Save(const std::filesystem::path& oPath, AssetTypeId oType, uint32_t uFormatVersion, std::span<const std::byte> spanPayload)
    {
        const std::vector<std::byte> vBytes = Encode(oType, uFormatVersion, spanPayload);
        Core::FileBytes::WriteAtomically(oPath, vBytes);
    }
}
