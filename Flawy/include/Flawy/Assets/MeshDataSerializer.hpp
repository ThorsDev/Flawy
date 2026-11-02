#pragma once

#include "Flawy/Assets/MeshData.hpp"

#include <cstddef>
#include <filesystem>
#include <span>
#include <vector>

namespace Flawy::Assets
{
    class MeshDataSerializer final
    {
    public:
        MeshDataSerializer() = delete;

        [[nodiscard]] static std::vector<std::byte> Encode(const MeshData& oMesh);
        [[nodiscard]] static MeshData Decode(std::span<const std::byte> spanFile);
        static void Save(const std::filesystem::path& oPath, const MeshData& oMesh);
    };
}
