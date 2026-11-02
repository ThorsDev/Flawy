#include "Flawy/Animation/ModelPose.hpp"

#include "Animation/Ozz/OzzLocalPoseImpl.hpp"
#include "Animation/Ozz/OzzMathConversion.hpp"
#include "Animation/Ozz/OzzSkeletonImpl.hpp"
#include "Flawy/Animation/LocalPose.hpp"
#include "Flawy/Animation/Skeleton.hpp"

#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/span.h>

#include <algorithm>
#include <format>
#include <stdexcept>
#include <vector>

namespace Flawy::Animation
{
    struct ModelPose::Impl
    {
        explicit Impl(const Skeleton& oOwner)
            : oSkeleton(oOwner)
            , vScratch(oOwner.GetJointCount())
            , vJointMatrices(oOwner.GetModelRestPose().begin(), oOwner.GetModelRestPose().end())
            , vSkinningMatrices(oOwner.GetJointCount())
        {
            ComputeSkinning();
        }

        void ComputeSkinning() noexcept
        {
            const std::span<const glm::mat4> spanInverseBind = oSkeleton.GetInverseBindMatrices();
            for (size_t uJoint = 0; uJoint < vJointMatrices.size(); ++uJoint)
            {
                vSkinningMatrices[uJoint] = vJointMatrices[uJoint] * spanInverseBind[uJoint];
            }
        }

        const Skeleton& oSkeleton;
        ozz::vector<ozz::math::Float4x4> vScratch;
        std::vector<glm::mat4> vJointMatrices;
        std::vector<glm::mat4> vSkinningMatrices;
    };

    ModelPose::ModelPose(const Skeleton& oSkeleton)
        : m_upImpl(std::make_unique<Impl>(oSkeleton))
    {
    }

    ModelPose::~ModelPose() = default;

    void ModelPose::Update(const LocalPose& oLocalPose)
    {
        Impl& oImpl = *m_upImpl;
        if (&oLocalPose.GetSkeleton() != &oImpl.oSkeleton)
        {
            throw std::logic_error(std::format("Model pose of skeleton '{}' cannot be updated from a local pose of skeleton '{}'",
                                               oImpl.oSkeleton.GetName(),
                                               oLocalPose.GetSkeleton().GetName()));
        }

        ozz::animation::LocalToModelJob oJob;
        oJob.skeleton = oImpl.oSkeleton.GetImpl().upSkeleton.get();
        oJob.input = ozz::make_span(oLocalPose.GetImpl().vTransforms);
        oJob.output = ozz::make_span(oImpl.vScratch);
        if (!oJob.Run())
        {
            throw std::logic_error(std::format("Local-to-model job failed for skeleton '{}'", oImpl.oSkeleton.GetName()));
        }

        for (size_t uJoint = 0; uJoint < oImpl.vJointMatrices.size(); ++uJoint)
        {
            oImpl.vJointMatrices[uJoint] = OzzMathConversion::ToGlm(oImpl.vScratch[uJoint]);
        }
        oImpl.ComputeSkinning();
    }

    std::span<const glm::mat4> ModelPose::GetJointMatrices() const noexcept
    {
        return m_upImpl->vJointMatrices;
    }

    std::span<const glm::mat4> ModelPose::GetSkinningMatrices() const noexcept
    {
        return m_upImpl->vSkinningMatrices;
    }

    const Skeleton& ModelPose::GetSkeleton() const noexcept
    {
        return m_upImpl->oSkeleton;
    }
}
