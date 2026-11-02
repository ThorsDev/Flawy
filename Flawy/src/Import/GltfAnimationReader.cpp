#include "Import/GltfAnimationReader.hpp"

#include "Import/GltfAccessorReader.hpp"
#include "Import/GltfSkinReader.hpp"
#include "Import/KeyframeResampler.hpp"
#include "Import/SkeletonAssembler.hpp"

#include <fastgltf/glm_element_traits.hpp>

#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <algorithm>
#include <cmath>
#include <format>
#include <limits>
#include <stdexcept>
#include <utility>

namespace Flawy::Import
{
    namespace
    {
        constexpr float kMinimumClipDuration = 1.0f / KeyframeResampler::kCubicSplineSampleRate;

        glm::quat ToQuaternion(const glm::vec4& vecValue) noexcept
        {
            return glm::quat(vecValue.w, vecValue.x, vecValue.y, vecValue.z);
        }

        template <typename TValue>
        std::vector<TimedValue<TValue>> BuildKeys(std::string_view svAnimation,
                                                  size_t uChannel,
                                                  const std::vector<float>& vTimes,
                                                  const std::vector<TValue>& vValues,
                                                  fastgltf::AnimationInterpolation eInterpolation)
        {
            const bool bCubic = eInterpolation == fastgltf::AnimationInterpolation::CubicSpline;
            const size_t uExpected = bCubic ? vTimes.size() * 3 : vTimes.size();
            if (vValues.size() != uExpected)
            {
                throw std::runtime_error(std::format("animation '{}' channel {} has {} output values, expected {}", svAnimation, uChannel, vValues.size(), uExpected));
            }
            if (bCubic)
            {
                return KeyframeResampler::ResampleCubicSpline(vTimes, vValues);
            }
            if (eInterpolation == fastgltf::AnimationInterpolation::Step)
            {
                return KeyframeResampler::ExpandStep(vTimes, vValues);
            }
            return KeyframeResampler::CopyLinear(vTimes, vValues);
        }

        template <typename TKey>
        void OffsetKeys(std::vector<TKey>& vKeys, float fStart) noexcept
        {
            for (TKey& oKey : vKeys)
            {
                oKey.fTimeSeconds -= fStart;
            }
        }
    }

    GltfAnimationReader::GltfAnimationReader(const fastgltf::Asset& oAsset,
                                             const GltfAccessorReader& oAccessors,
                                             const GltfSkinReader& oSkin,
                                             std::vector<std::string>& vWarnings)
        : m_oAsset(oAsset)
        , m_oAccessors(oAccessors)
        , m_oSkin(oSkin)
        , m_vWarnings(vWarnings)
    {
    }

    std::optional<Animation::AnimationClipDescriptor> GltfAnimationReader::Read(size_t uAnimation)
    {
        const fastgltf::Animation& oAnimation = m_oAsset.animations.at(uAnimation);
        const std::string sSourceName(std::string_view(oAnimation.name));
        const std::string sName = sSourceName.empty() ? std::format("Animation{}", uAnimation) : sSourceName;

        std::map<uint32_t, JointTrackKeys> mapTracks;
        uint32_t uOutsideChannels = 0;
        bool bResampled = false;
        for (size_t uChannel = 0; uChannel < oAnimation.channels.size(); ++uChannel)
        {
            const fastgltf::AnimationChannel& oChannel = oAnimation.channels[uChannel];
            if (oChannel.path == fastgltf::AnimationPath::Weights)
            {
                if (!m_bWarnedWeights)
                {
                    m_vWarnings.emplace_back("morph target weight animation is not supported and was ignored");
                    m_bWarnedWeights = true;
                }
                continue;
            }
            const std::optional<uint32_t> optJoint = oChannel.nodeIndex ? m_oSkin.FindJoint(*oChannel.nodeIndex) : std::nullopt;
            if (!optJoint)
            {
                ++uOutsideChannels;
                continue;
            }
            if (oChannel.samplerIndex >= oAnimation.samplers.size())
            {
                throw std::runtime_error(std::format("animation '{}' channel {} references missing sampler {}", sName, uChannel, oChannel.samplerIndex));
            }
            const fastgltf::AnimationSampler& oSampler = oAnimation.samplers[oChannel.samplerIndex];
            bResampled = bResampled || oSampler.interpolation == fastgltf::AnimationInterpolation::CubicSpline;
            ReadChannel(sName, uChannel, oChannel.path, oSampler, *optJoint, mapTracks[*optJoint]);
        }

        if (uOutsideChannels != 0)
        {
            m_vWarnings.push_back(std::format("animation '{}': {} channel(s) target nodes outside the skeleton and were ignored", sName, uOutsideChannels));
        }
        if (bResampled)
        {
            m_vWarnings.push_back(std::format("animation '{}' uses cubic spline interpolation; it was resampled at {} Hz", sName, KeyframeResampler::kCubicSplineSampleRate));
        }
        std::erase_if(mapTracks, [](const auto& oEntry) {
            const JointTrackKeys& oTrack = oEntry.second;
            return oTrack.vTranslations.empty() && oTrack.vRotations.empty() && oTrack.vScales.empty();
        });
        if (mapTracks.empty())
        {
            m_vWarnings.push_back(std::format("animation '{}' does not animate any skeleton joint and was skipped", sName));
            return std::nullopt;
        }
        return BuildClip(sName, mapTracks);
    }

    void GltfAnimationReader::ReadChannel(std::string_view svAnimation,
                                          size_t uChannel,
                                          fastgltf::AnimationPath ePath,
                                          const fastgltf::AnimationSampler& oSampler,
                                          uint32_t uJoint,
                                          JointTrackKeys& oTrack)
    {
        const std::vector<float> vTimes = ReadKeyTimes(svAnimation, uChannel, oSampler.inputAccessor);
        if (vTimes.empty())
        {
            return;
        }

        bool& bPresent = ePath == fastgltf::AnimationPath::Translation ? oTrack.bTranslation
                         : ePath == fastgltf::AnimationPath::Rotation  ? oTrack.bRotation
                                                                        : oTrack.bScale;
        if (bPresent)
        {
            m_vWarnings.push_back(std::format("animation '{}' channel {} duplicates an earlier channel and was ignored", svAnimation, uChannel));
            return;
        }
        bPresent = true;

        const SkeletonAssembler& oSkeleton = m_oSkin.GetSkeleton();
        if (ePath == fastgltf::AnimationPath::Rotation)
        {
            const std::vector<glm::vec4> vValues = m_oAccessors.Read<glm::vec4>(oSampler.outputAccessor, "animation output");
            for (const TimedValue<glm::vec4>& oKey : BuildKeys(svAnimation, uChannel, vTimes, vValues, oSampler.interpolation))
            {
                oTrack.vRotations.push_back({oKey.fTime, oSkeleton.ToJointRotation(uJoint, glm::normalize(ToQuaternion(oKey.oValue)))});
            }
            return;
        }

        const std::vector<glm::vec3> vValues = m_oAccessors.Read<glm::vec3>(oSampler.outputAccessor, "animation output");
        for (const TimedValue<glm::vec3>& oKey : BuildKeys(svAnimation, uChannel, vTimes, vValues, oSampler.interpolation))
        {
            if (ePath == fastgltf::AnimationPath::Translation)
            {
                oTrack.vTranslations.push_back({oKey.fTime, oSkeleton.ToJointTranslation(uJoint, oKey.oValue)});
            }
            else
            {
                oTrack.vScales.push_back({oKey.fTime, oSkeleton.ToJointScale(uJoint, oKey.oValue)});
            }
        }
    }

    std::vector<float> GltfAnimationReader::ReadKeyTimes(std::string_view svAnimation, size_t uChannel, size_t uAccessor) const
    {
        const std::vector<float> vTimes = m_oAccessors.Read<float>(uAccessor, "animation input");
        for (size_t uKey = 0; uKey < vTimes.size(); ++uKey)
        {
            if (!std::isfinite(vTimes[uKey]) || (uKey != 0 && vTimes[uKey] <= vTimes[uKey - 1]))
            {
                throw std::runtime_error(std::format("animation '{}' channel {} key times are not finite and strictly increasing", svAnimation, uChannel));
            }
        }
        return vTimes;
    }

    Animation::AnimationClipDescriptor GltfAnimationReader::BuildClip(const std::string& sName, std::map<uint32_t, JointTrackKeys>& mapTracks) const
    {
        float fStart = std::numeric_limits<float>::max();
        float fEnd = std::numeric_limits<float>::lowest();
        const auto fnExtend = [&fStart, &fEnd](float fTime) {
            fStart = std::min(fStart, fTime);
            fEnd = std::max(fEnd, fTime);
        };
        for (const auto& [uJoint, oTrack] : mapTracks)
        {
            for (const Animation::Vec3Key& oKey : oTrack.vTranslations)
            {
                fnExtend(oKey.fTimeSeconds);
            }
            for (const Animation::QuatKey& oKey : oTrack.vRotations)
            {
                fnExtend(oKey.fTimeSeconds);
            }
            for (const Animation::Vec3Key& oKey : oTrack.vScales)
            {
                fnExtend(oKey.fTimeSeconds);
            }
        }

        Animation::AnimationClipDescriptor oClip;
        oClip.sName = sName;
        oClip.fDurationSeconds = std::max(fEnd - fStart, kMinimumClipDuration);
        for (auto& [uJoint, oTrack] : mapTracks)
        {
            Animation::JointTrackDescriptor& oTarget = oClip.AddTrack(m_oSkin.GetSkeleton().GetJointName(uJoint));
            OffsetKeys(oTrack.vTranslations, fStart);
            OffsetKeys(oTrack.vRotations, fStart);
            OffsetKeys(oTrack.vScales, fStart);
            oTarget.vTranslations = std::move(oTrack.vTranslations);
            oTarget.vRotations = std::move(oTrack.vRotations);
            oTarget.vScales = std::move(oTrack.vScales);
        }
        return oClip;
    }
}
