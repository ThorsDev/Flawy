#include "Animation/Ozz/OzzRawSkeletonBuilder.hpp"

#include "Animation/Ozz/OzzMathConversion.hpp"

namespace Flawy::Animation
{
    OzzRawSkeletonBuilder::OzzRawSkeletonBuilder(const SkeletonDescriptor& oDescriptor)
        : m_oDescriptor(oDescriptor)
        , m_vChildren(oDescriptor.vJoints.size())
    {
        for (size_t uJoint = 0; uJoint < oDescriptor.vJoints.size(); ++uJoint)
        {
            const int32_t iParent = oDescriptor.vJoints[uJoint].iParentIndex;
            if (iParent == SkeletonDescriptor::kNoParent)
            {
                m_vRoots.push_back(static_cast<uint32_t>(uJoint));
            }
            else
            {
                m_vChildren[static_cast<size_t>(iParent)].push_back(static_cast<uint32_t>(uJoint));
            }
        }
    }

    ozz::animation::offline::RawSkeleton OzzRawSkeletonBuilder::Build() const
    {
        ozz::animation::offline::RawSkeleton oRaw;
        AppendChildren(oRaw.roots, SkeletonDescriptor::kNoParent);
        return oRaw;
    }

    void OzzRawSkeletonBuilder::AppendChildren(ozz::animation::offline::RawSkeleton::Joint::Children& vTarget, int32_t iParentIndex) const
    {
        const bool bRoots = iParentIndex == SkeletonDescriptor::kNoParent;
        const std::vector<uint32_t>& vSource = bRoots ? m_vRoots : m_vChildren[static_cast<size_t>(iParentIndex)];
        vTarget.resize(vSource.size());
        for (size_t uChild = 0; uChild < vSource.size(); ++uChild)
        {
            const JointDescriptor& oJoint = m_oDescriptor.vJoints[vSource[uChild]];
            ozz::animation::offline::RawSkeleton::Joint& oRawJoint = vTarget[uChild];
            oRawJoint.name.assign(oJoint.sName.data(), oJoint.sName.size());
            oRawJoint.transform = OzzMathConversion::ToOzz(oJoint.oLocalRestPose);
            AppendChildren(oRawJoint.children, static_cast<int32_t>(vSource[uChild]));
        }
    }
}
