#pragma once

#include "Flawy/Animation/AnimationClipDescriptor.hpp"
#include "Flawy/Animation/SkeletonDescriptor.hpp"
#include "Flawy/Assets/MeshData.hpp"

#include <optional>
#include <string>
#include <vector>

namespace Flawy::Import
{
    struct ImportedScene
    {
        std::vector<Assets::MeshData> vMeshes;
        std::optional<Animation::SkeletonDescriptor> optSkeleton;
        std::vector<Animation::AnimationClipDescriptor> vClips;
        std::vector<std::string> vWarnings;
    };
}
