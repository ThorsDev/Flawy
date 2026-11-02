#pragma once

#include "Flawy/Animation/AnimationClipDescriptor.hpp"

#include <fastgltf/types.hpp>

#include <cstddef>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Flawy::Import
{
    class GltfAccessorReader;
    class GltfSkinReader;

    class GltfAnimationReader final
    {
    public:
        GltfAnimationReader(const fastgltf::Asset& oAsset, const GltfAccessorReader& oAccessors, const GltfSkinReader& oSkin, std::vector<std::string>& vWarnings);

        [[nodiscard]] std::optional<Animation::AnimationClipDescriptor> Read(size_t uAnimation);

    private:
        struct JointTrackKeys
        {
            std::vector<Animation::Vec3Key> vTranslations;
            std::vector<Animation::QuatKey> vRotations;
            std::vector<Animation::Vec3Key> vScales;
            bool bTranslation = false;
            bool bRotation = false;
            bool bScale = false;
        };

        void ReadChannel(std::string_view svAnimation,
                         size_t uChannel,
                         fastgltf::AnimationPath ePath,
                         const fastgltf::AnimationSampler& oSampler,
                         uint32_t uJoint,
                         JointTrackKeys& oTrack);
        [[nodiscard]] std::vector<float> ReadKeyTimes(std::string_view svAnimation, size_t uChannel, size_t uAccessor) const;
        [[nodiscard]] Animation::AnimationClipDescriptor BuildClip(const std::string& sName, std::map<uint32_t, JointTrackKeys>& mapTracks) const;

        const fastgltf::Asset& m_oAsset;
        const GltfAccessorReader& m_oAccessors;
        const GltfSkinReader& m_oSkin;
        std::vector<std::string>& m_vWarnings;
        bool m_bWarnedWeights = false;
    };
}
