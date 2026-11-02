#pragma once

#include "Flawy/Assets/AssetId.hpp"

#include <cstddef>
#include <filesystem>
#include <span>
#include <string>

namespace Flawy::Assets
{
    class AssetManager;

    struct AssetLoadContext
    {
        AssetId oId;
        std::string sName;
        std::filesystem::path oAbsolutePath;
        std::span<const std::byte> spanBytes;
        AssetManager& oManager;
    };
}
