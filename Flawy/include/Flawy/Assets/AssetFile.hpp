#pragma once

#include "Flawy/Assets/AssetTypeId.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

namespace Flawy::Assets
{
    class AssetFile final
    {
    public:
        static constexpr std::string_view kMagic = "FLAW";
        static constexpr uint32_t kHeaderVersion = 1;
        static constexpr size_t kFormatVersionOffset = 16;
        static constexpr size_t kPayloadOffset = 28;
        static constexpr size_t kChecksumSize = 8;

        struct Contents
        {
            uint32_t uFormatVersion = 0;
            std::span<const std::byte> spanPayload;
        };

        AssetFile() = delete;

        [[nodiscard]] static std::vector<std::byte> Encode(AssetTypeId oType, uint32_t uFormatVersion, std::span<const std::byte> spanPayload);
        [[nodiscard]] static Contents Decode(std::span<const std::byte> spanFile, AssetTypeId oExpectedType);
        static void Save(const std::filesystem::path& oPath, AssetTypeId oType, uint32_t uFormatVersion, std::span<const std::byte> spanPayload);
    };
}
