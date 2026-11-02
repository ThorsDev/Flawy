#pragma once

#include "Flawy/Animation/AnimationClipDescriptor.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <span>
#include <vector>

namespace Flawy::Animation
{
    class AnimationClipDescriptorSerializer final
    {
    public:
        static constexpr uint32_t kFormatVersion = 1;

        AnimationClipDescriptorSerializer() = delete;

        [[nodiscard]] static std::vector<std::byte> Encode(const AnimationClipDescriptor& oClip);
        [[nodiscard]] static AnimationClipDescriptor Decode(std::span<const std::byte> spanFile);
        static void Save(const std::filesystem::path& oPath, const AnimationClipDescriptor& oClip);
    };
}
