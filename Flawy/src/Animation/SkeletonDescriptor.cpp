#include "Flawy/Animation/SkeletonDescriptor.hpp"

#include "Animation/AnimationValueChecks.hpp"

#include <format>
#include <stdexcept>
#include <string_view>
#include <unordered_set>
#include <utility>

namespace Flawy::Animation
{
    namespace
    {
        class SkeletonDescriptorValidator final
        {
        public:
            explicit SkeletonDescriptorValidator(const SkeletonDescriptor& oDescriptor) noexcept
                : m_oDescriptor(oDescriptor)
            {
            }

            void Validate() const
            {
                if (m_oDescriptor.sName.size() > SkeletonDescriptor::kMaxNameLength)
                {
                    Fail(std::format("has a name longer than {} characters", SkeletonDescriptor::kMaxNameLength));
                }
                if (m_oDescriptor.vJoints.empty())
                {
                    Fail("must contain at least one joint");
                }
                if (m_oDescriptor.vJoints.size() > SkeletonDescriptor::kMaxJoints)
                {
                    Fail(std::format("has {} joints, the maximum is {}", m_oDescriptor.vJoints.size(), SkeletonDescriptor::kMaxJoints));
                }

                std::unordered_set<std::string_view> setNames;
                for (size_t uJoint = 0; uJoint < m_oDescriptor.vJoints.size(); ++uJoint)
                {
                    const JointDescriptor& oJoint = m_oDescriptor.vJoints[uJoint];
                    ValidateJoint(oJoint, uJoint);
                    if (!setNames.insert(oJoint.sName).second)
                    {
                        FailJoint(uJoint, oJoint, "has a duplicate name");
                    }
                }
            }

        private:
            void ValidateJoint(const JointDescriptor& oJoint, size_t uJoint) const
            {
                if (oJoint.sName.empty())
                {
                    FailJoint(uJoint, oJoint, "has an empty name");
                }
                if (oJoint.sName.size() > SkeletonDescriptor::kMaxNameLength)
                {
                    FailJoint(uJoint, oJoint, std::format("has a name longer than {} characters", SkeletonDescriptor::kMaxNameLength));
                }
                const bool bRoot = oJoint.iParentIndex == SkeletonDescriptor::kNoParent;
                if (!bRoot && (oJoint.iParentIndex < 0 || static_cast<size_t>(oJoint.iParentIndex) >= uJoint))
                {
                    FailJoint(uJoint, oJoint, std::format("has parent index {}; a parent must be {} or an earlier joint",
                                                          oJoint.iParentIndex,
                                                          SkeletonDescriptor::kNoParent));
                }

                const JointTransform& oRest = oJoint.oLocalRestPose;
                if (!AnimationValueChecks::IsFinite(oRest.vecTranslation) || !AnimationValueChecks::IsFinite(oRest.vecScale) ||
                    !AnimationValueChecks::IsFinite(oRest.quatRotation))
                {
                    FailJoint(uJoint, oJoint, "has a non-finite rest pose");
                }
                if (!AnimationValueChecks::IsUsableRotation(oRest.quatRotation))
                {
                    FailJoint(uJoint, oJoint, "has a zero-length rest rotation");
                }
                if (oJoint.optInverseBindMatrix && !AnimationValueChecks::IsFinite(*oJoint.optInverseBindMatrix))
                {
                    FailJoint(uJoint, oJoint, "has a non-finite inverse bind matrix");
                }
            }

            [[noreturn]] void Fail(std::string_view svReason) const
            {
                throw std::invalid_argument(std::format("Skeleton '{}' {}", m_oDescriptor.sName, svReason));
            }

            [[noreturn]] void FailJoint(size_t uJoint, const JointDescriptor& oJoint, std::string_view svReason) const
            {
                throw std::invalid_argument(std::format("Skeleton '{}': joint {} ('{}') {}",
                                                        m_oDescriptor.sName,
                                                        uJoint,
                                                        oJoint.sName,
                                                        svReason));
            }

            const SkeletonDescriptor& m_oDescriptor;
        };
    }

    uint32_t SkeletonDescriptor::AddJoint(std::string sJointName, int32_t iParentIndex, const JointTransform& oLocalRestPose)
    {
        JointDescriptor oJoint;
        oJoint.sName = std::move(sJointName);
        oJoint.iParentIndex = iParentIndex;
        oJoint.oLocalRestPose = oLocalRestPose;
        vJoints.push_back(std::move(oJoint));
        return static_cast<uint32_t>(vJoints.size() - 1);
    }

    void SkeletonDescriptor::Validate() const
    {
        SkeletonDescriptorValidator(*this).Validate();
    }
}
