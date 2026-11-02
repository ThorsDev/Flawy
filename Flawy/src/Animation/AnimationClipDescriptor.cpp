#include "Flawy/Animation/AnimationClipDescriptor.hpp"

#include "Animation/AnimationValueChecks.hpp"

#include <cmath>
#include <format>
#include <span>
#include <stdexcept>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace Flawy::Animation
{
    namespace
    {
        class AnimationClipDescriptorValidator final
        {
        public:
            explicit AnimationClipDescriptorValidator(const AnimationClipDescriptor& oDescriptor) noexcept
                : m_oDescriptor(oDescriptor)
            {
            }

            void Validate() const
            {
                if (m_oDescriptor.sName.empty())
                {
                    Fail("must have a name");
                }
                if (m_oDescriptor.sName.size() > AnimationClipDescriptor::kMaxNameLength)
                {
                    Fail(std::format("name is longer than {} characters", AnimationClipDescriptor::kMaxNameLength));
                }
                if (m_oDescriptor.sSkeletonPath.size() > AnimationClipDescriptor::kMaxNameLength)
                {
                    Fail(std::format("skeleton path is longer than {} characters", AnimationClipDescriptor::kMaxNameLength));
                }
                if (!std::isfinite(m_oDescriptor.fDurationSeconds) || m_oDescriptor.fDurationSeconds <= 0.0f)
                {
                    Fail(std::format("duration {} must be a finite number greater than zero", m_oDescriptor.fDurationSeconds));
                }
                if (m_oDescriptor.vTracks.size() > AnimationClipDescriptor::kMaxTracks)
                {
                    Fail(std::format("has {} tracks, the maximum is {}",
                                     m_oDescriptor.vTracks.size(),
                                     AnimationClipDescriptor::kMaxTracks));
                }

                std::unordered_set<std::string_view> setJoints;
                for (const JointTrackDescriptor& oTrack : m_oDescriptor.vTracks)
                {
                    if (oTrack.sJointName.empty())
                    {
                        Fail("has a track with an empty joint name");
                    }
                    if (!setJoints.insert(oTrack.sJointName).second)
                    {
                        FailTrack(oTrack, "is duplicated");
                    }
                    ValidateKeys(oTrack, "translation", std::span<const Vec3Key>(oTrack.vTranslations));
                    ValidateKeys(oTrack, "rotation", std::span<const QuatKey>(oTrack.vRotations));
                    ValidateKeys(oTrack, "scale", std::span<const Vec3Key>(oTrack.vScales));
                }
            }

        private:
            template <typename TKey>
            void ValidateKeys(const JointTrackDescriptor& oTrack, std::string_view svComponent, std::span<const TKey> spanKeys) const
            {
                if (spanKeys.size() > AnimationClipDescriptor::kMaxKeysPerComponent)
                {
                    FailTrack(oTrack, std::format("has {} {} keys, the maximum is {}",
                                                  spanKeys.size(),
                                                  svComponent,
                                                  AnimationClipDescriptor::kMaxKeysPerComponent));
                }

                float fPreviousTime = -1.0f;
                for (size_t uKey = 0; uKey < spanKeys.size(); ++uKey)
                {
                    const TKey& oKey = spanKeys[uKey];
                    if (!std::isfinite(oKey.fTimeSeconds) || oKey.fTimeSeconds < 0.0f || oKey.fTimeSeconds > m_oDescriptor.fDurationSeconds)
                    {
                        FailKey(oTrack, svComponent, uKey, std::format("time {} is outside [0, {}]",
                                                                       oKey.fTimeSeconds,
                                                                       m_oDescriptor.fDurationSeconds));
                    }
                    if (oKey.fTimeSeconds <= fPreviousTime)
                    {
                        FailKey(oTrack, svComponent, uKey, "time is not strictly ascending");
                    }
                    ValidateValue(oTrack, svComponent, uKey, oKey);
                    fPreviousTime = oKey.fTimeSeconds;
                }
            }

            void ValidateValue(const JointTrackDescriptor& oTrack, std::string_view svComponent, size_t uKey, const Vec3Key& oKey) const
            {
                if (!AnimationValueChecks::IsFinite(oKey.vecValue))
                {
                    FailKey(oTrack, svComponent, uKey, "value is not finite");
                }
            }

            void ValidateValue(const JointTrackDescriptor& oTrack, std::string_view svComponent, size_t uKey, const QuatKey& oKey) const
            {
                if (!AnimationValueChecks::IsFinite(oKey.quatValue))
                {
                    FailKey(oTrack, svComponent, uKey, "value is not finite");
                }
                if (!AnimationValueChecks::IsUsableRotation(oKey.quatValue))
                {
                    FailKey(oTrack, svComponent, uKey, "rotation has zero length");
                }
            }

            [[noreturn]] void Fail(std::string_view svReason) const
            {
                throw std::invalid_argument(std::format("Animation clip '{}' {}", m_oDescriptor.sName, svReason));
            }

            [[noreturn]] void FailTrack(const JointTrackDescriptor& oTrack, std::string_view svReason) const
            {
                throw std::invalid_argument(std::format("Animation clip '{}': track '{}' {}",
                                                        m_oDescriptor.sName,
                                                        oTrack.sJointName,
                                                        svReason));
            }

            [[noreturn]] void FailKey(const JointTrackDescriptor& oTrack,
                                      std::string_view svComponent,
                                      size_t uKey,
                                      std::string_view svReason) const
            {
                throw std::invalid_argument(
                    std::format("Animation clip '{}': track '{}' {} key {}: {}",
                                m_oDescriptor.sName,
                                oTrack.sJointName,
                                svComponent,
                                uKey,
                                svReason));
            }

            const AnimationClipDescriptor& m_oDescriptor;
        };
    }

    JointTrackDescriptor& AnimationClipDescriptor::AddTrack(std::string sJointName)
    {
        JointTrackDescriptor oTrack;
        oTrack.sJointName = std::move(sJointName);
        return vTracks.emplace_back(std::move(oTrack));
    }

    void AnimationClipDescriptor::Validate() const
    {
        AnimationClipDescriptorValidator(*this).Validate();
    }
}
