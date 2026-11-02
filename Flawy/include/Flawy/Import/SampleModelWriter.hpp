#pragma once

#include <filesystem>
#include <string_view>

namespace Flawy::Import
{
    class SampleModelWriter final
    {
    public:
        static constexpr std::string_view kTubeMeshName = "TubeMesh";
        static constexpr std::string_view kTubeSkeletonName = "Tube";
        static constexpr std::string_view kTubeClipName = "Bend";

        SampleModelWriter() = delete;

        static void WriteSkinnedTubeGlb(const std::filesystem::path& oPath);
    };
}
