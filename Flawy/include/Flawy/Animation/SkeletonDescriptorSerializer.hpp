#pragma once

#include "Flawy/Animation/SkeletonDescriptor.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace Flawy::Animation
{
    class SkeletonDescriptorSerializer final
    {
    public:
        static constexpr uint32_t kFormatVersion = 1;

        SkeletonDescriptorSerializer() = delete;

        [[nodiscard]] static std::vector<std::byte> Encode(const SkeletonDescriptor& oSkeleton);
        [[nodiscard]] static SkeletonDescriptor Decode(std::span<const std::byte> spanFile);
        static void Save(const std::filesystem::path& oPath, const SkeletonDescriptor& oSkeleton);
    };
}
