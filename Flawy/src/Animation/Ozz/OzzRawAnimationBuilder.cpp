#include "Animation/Ozz/OzzRawAnimationBuilder.hpp"

#include "Animation/Ozz/OzzMathConversion.hpp"
#include "Flawy/Animation/Skeleton.hpp"

#include <format>
#include <optional>
#include <stdexcept>

namespace Flawy::Animation
{
    OzzRawAnimationBuilder::OzzRawAnimationBuilder(const Skeleton& oSkeleton, const AnimationClipDescriptor& oDescriptor)
        : m_oSkeleton(oSkeleton)
        , m_oDescriptor(oDescriptor)
        , m_vTracksByJoint(oSkeleton.GetJointCount(), nullptr)
    {
        for (const JointTrackDescriptor& oTrack : oDescriptor.vTracks)
        {
            const std::optional<uint32_t> optJoint = oSkeleton.FindJoint(oTrack.sJointName);
            if (!optJoint)
            {
                throw std::invalid_argument(std::format("Animation clip '{}': track '{}' targets a joint that is not in skeleton '{}'",
                                                        oDescriptor.sName,
                                                        oTrack.sJointName,
                                                        oSkeleton.GetName()));
            }
            m_vTracksByJoint[*optJoint] = &oTrack;
        }
    }

    ozz::animation::offline::RawAnimation OzzRawAnimationBuilder::Build() const
    {
        ozz::animation::offline::RawAnimation oRaw;
        oRaw.duration = m_oDescriptor.fDurationSeconds;
        oRaw.name.assign(m_oDescriptor.sName.data(), m_oDescriptor.sName.size());
        oRaw.tracks.resize(m_oSkeleton.GetJointCount());
        for (uint32_t uJoint = 0; uJoint < m_oSkeleton.GetJointCount(); ++uJoint)
        {
            FillTrack(oRaw.tracks[uJoint], uJoint);
        }
        return oRaw;
    }

    void OzzRawAnimationBuilder::FillTrack(ozz::animation::offline::RawAnimation::JointTrack& oTrack, uint32_t uJoint) const
    {
        using RawAnimation = ozz::animation::offline::RawAnimation;

        const JointTransform& oRest = m_oSkeleton.GetLocalRestPose(uJoint);
        const JointTrackDescriptor* pSource = m_vTracksByJoint[uJoint];

        if (pSource != nullptr && !pSource->vTranslations.empty())
        {
            for (const Vec3Key& oKey : pSource->vTranslations)
            {
                oTrack.translations.push_back(RawAnimation::TranslationKey{oKey.fTimeSeconds, OzzMathConversion::ToOzz(oKey.vecValue)});
            }
        }
        else
        {
            oTrack.translations.push_back(RawAnimation::TranslationKey{0.0f, OzzMathConversion::ToOzz(oRest.vecTranslation)});
        }

        if (pSource != nullptr && !pSource->vRotations.empty())
        {
            for (const QuatKey& oKey : pSource->vRotations)
            {
                oTrack.rotations.push_back(RawAnimation::RotationKey{oKey.fTimeSeconds, OzzMathConversion::ToOzz(oKey.quatValue)});
            }
        }
        else
        {
            oTrack.rotations.push_back(RawAnimation::RotationKey{0.0f, OzzMathConversion::ToOzz(oRest.quatRotation)});
        }

        if (pSource != nullptr && !pSource->vScales.empty())
        {
            for (const Vec3Key& oKey : pSource->vScales)
            {
                oTrack.scales.push_back(RawAnimation::ScaleKey{oKey.fTimeSeconds, OzzMathConversion::ToOzz(oKey.vecValue)});
            }
        }
        else
        {
            oTrack.scales.push_back(RawAnimation::ScaleKey{0.0f, OzzMathConversion::ToOzz(oRest.vecScale)});
        }
    }
}
