#include "Flawy/Animation/LocalPose.hpp"

#include "Animation/Ozz/OzzLocalPoseImpl.hpp"
#include "Animation/Ozz/OzzSkeletonImpl.hpp"

#include <algorithm>

namespace Flawy::Animation
{
    LocalPose::LocalPose(const Skeleton& oSkeleton)
        : m_upImpl(std::make_unique<Impl>())
    {
        m_upImpl->pSkeleton = &oSkeleton;
        m_upImpl->vTransforms.resize(static_cast<size_t>(oSkeleton.GetImpl().upSkeleton->num_soa_joints()));
        ResetToRestPose();
    }

    LocalPose::~LocalPose() = default;

    void LocalPose::ResetToRestPose() noexcept
    {
        const auto spanRest = m_upImpl->pSkeleton->GetImpl().upSkeleton->joint_rest_poses();
        std::copy(spanRest.begin(), spanRest.end(), m_upImpl->vTransforms.begin());
    }

    const Skeleton& LocalPose::GetSkeleton() const noexcept
    {
        return *m_upImpl->pSkeleton;
    }

    LocalPose::Impl& LocalPose::GetImpl() noexcept
    {
        return *m_upImpl;
    }

    const LocalPose::Impl& LocalPose::GetImpl() const noexcept
    {
        return *m_upImpl;
    }
}
