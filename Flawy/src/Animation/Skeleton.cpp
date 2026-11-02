#include "Flawy/Animation/Skeleton.hpp"

#include "Animation/AnimationValueChecks.hpp"
#include "Animation/Ozz/OzzMathConversion.hpp"
#include "Animation/Ozz/OzzRawSkeletonBuilder.hpp"
#include "Animation/Ozz/OzzSkeletonImpl.hpp"
#include "Flawy/Animation/SkeletonDescriptor.hpp"

#include <glm/matrix.hpp>
#include <ozz/animation/offline/skeleton_builder.h>
#include <ozz/animation/runtime/local_to_model_job.h>
#include <ozz/base/containers/vector.h>
#include <ozz/base/maths/simd_math.h>
#include <ozz/base/span.h>

#include <format>
#include <stdexcept>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Flawy::Animation
{
    namespace
    {
        class SkeletonImplBuilder final
        {
        public:
            SkeletonImplBuilder(const SkeletonDescriptor& oDescriptor, Skeleton::Impl& oImpl) noexcept
                : m_oDescriptor(oDescriptor)
                , m_oImpl(oImpl)
            {
            }

            void Build()
            {
                BuildRuntimeSkeleton();
                CopyJointData();
                ComputeModelRestPose();
                ComputeInverseBindMatrices();
            }

        private:
            void BuildRuntimeSkeleton()
            {
                const ozz::animation::offline::RawSkeleton oRaw = OzzRawSkeletonBuilder(m_oDescriptor).Build();
                if (!oRaw.Validate())
                {
                    throw std::logic_error(std::format("Skeleton '{}' produced an invalid ozz raw skeleton", m_oDescriptor.sName));
                }
                m_oImpl.upSkeleton = ozz::animation::offline::SkeletonBuilder()(oRaw);
                if (!m_oImpl.upSkeleton)
                {
                    throw std::logic_error(std::format("Skeleton '{}' could not be built", m_oDescriptor.sName));
                }
            }

            void CopyJointData()
            {
                std::unordered_map<std::string_view, size_t> mapDescriptorIndices;
                for (size_t uJoint = 0; uJoint < m_oDescriptor.vJoints.size(); ++uJoint)
                {
                    mapDescriptorIndices.emplace(m_oDescriptor.vJoints[uJoint].sName, uJoint);
                }

                const ozz::animation::Skeleton& oSkeleton = *m_oImpl.upSkeleton;
                const auto uJointCount = static_cast<size_t>(oSkeleton.num_joints());
                if (uJointCount != m_oDescriptor.vJoints.size())
                {
                    throw std::logic_error(std::format("Skeleton '{}' has {} runtime joints but {} were described",
                                                       m_oDescriptor.sName,
                                                       uJointCount,
                                                       m_oDescriptor.vJoints.size()));
                }

                m_oImpl.sName = m_oDescriptor.sName;
                m_vDescriptorIndices.reserve(uJointCount);
                m_oImpl.vJointNames.reserve(uJointCount);
                m_oImpl.vParentIndices.reserve(uJointCount);
                m_oImpl.vLocalRestPoses.reserve(uJointCount);
                for (size_t uJoint = 0; uJoint < uJointCount; ++uJoint)
                {
                    const std::string_view svName = oSkeleton.joint_names()[uJoint];
                    const size_t uDescriptorIndex = mapDescriptorIndices.at(svName);
                    const JointDescriptor& oJoint = m_oDescriptor.vJoints[uDescriptorIndex];

                    JointTransform oRest = oJoint.oLocalRestPose;
                    oRest.quatRotation = AnimationValueChecks::NormalizeRotation(oRest.quatRotation);

                    m_vDescriptorIndices.push_back(uDescriptorIndex);
                    m_oImpl.vJointNames.emplace_back(svName);
                    m_oImpl.vParentIndices.push_back(static_cast<int32_t>(oSkeleton.joint_parents()[uJoint]));
                    m_oImpl.vLocalRestPoses.push_back(oRest);
                    m_oImpl.mapJoints.emplace(std::string(svName), static_cast<uint32_t>(uJoint));
                }
            }

            void ComputeModelRestPose()
            {
                const ozz::animation::Skeleton& oSkeleton = *m_oImpl.upSkeleton;
                ozz::vector<ozz::math::Float4x4> vModel(static_cast<size_t>(oSkeleton.num_joints()));

                ozz::animation::LocalToModelJob oJob;
                oJob.skeleton = &oSkeleton;
                oJob.input = oSkeleton.joint_rest_poses();
                oJob.output = ozz::make_span(vModel);
                if (!oJob.Run())
                {
                    throw std::logic_error(std::format("Skeleton '{}' failed to compute its model-space rest pose", m_oDescriptor.sName));
                }

                m_oImpl.vModelRestPoses.reserve(vModel.size());
                for (const ozz::math::Float4x4& oMatrix : vModel)
                {
                    m_oImpl.vModelRestPoses.push_back(OzzMathConversion::ToGlm(oMatrix));
                }
            }

            void ComputeInverseBindMatrices()
            {
                m_oImpl.vInverseBindMatrices.reserve(m_oImpl.vModelRestPoses.size());
                for (size_t uJoint = 0; uJoint < m_oImpl.vModelRestPoses.size(); ++uJoint)
                {
                    const JointDescriptor& oJoint = m_oDescriptor.vJoints[m_vDescriptorIndices[uJoint]];
                    const glm::mat4& matModelRest = m_oImpl.vModelRestPoses[uJoint];
                    m_oImpl.vInverseBindMatrices.push_back(oJoint.optInverseBindMatrix.value_or(glm::inverse(matModelRest)));
                }
            }

            const SkeletonDescriptor& m_oDescriptor;
            Skeleton::Impl& m_oImpl;
            std::vector<size_t> m_vDescriptorIndices;
        };
    }

    Skeleton::Skeleton(const SkeletonDescriptor& oDescriptor)
        : m_upImpl(std::make_unique<Impl>())
    {
        oDescriptor.Validate();
        SkeletonImplBuilder(oDescriptor, *m_upImpl).Build();
    }

    Skeleton::~Skeleton() = default;

    const std::string& Skeleton::GetName() const noexcept
    {
        return m_upImpl->sName;
    }

    uint32_t Skeleton::GetJointCount() const noexcept
    {
        return static_cast<uint32_t>(m_upImpl->vJointNames.size());
    }

    const std::string& Skeleton::GetJointName(uint32_t uJoint) const
    {
        RequireJoint(uJoint);
        return m_upImpl->vJointNames[uJoint];
    }

    int32_t Skeleton::GetJointParentIndex(uint32_t uJoint) const
    {
        RequireJoint(uJoint);
        return m_upImpl->vParentIndices[uJoint];
    }

    std::optional<uint32_t> Skeleton::FindJoint(std::string_view svName) const noexcept
    {
        const auto itJoint = m_upImpl->mapJoints.find(svName);
        if (itJoint == m_upImpl->mapJoints.end())
        {
            return std::nullopt;
        }
        return itJoint->second;
    }

    const JointTransform& Skeleton::GetLocalRestPose(uint32_t uJoint) const
    {
        RequireJoint(uJoint);
        return m_upImpl->vLocalRestPoses[uJoint];
    }

    std::span<const glm::mat4> Skeleton::GetModelRestPose() const noexcept
    {
        return m_upImpl->vModelRestPoses;
    }

    std::span<const glm::mat4> Skeleton::GetInverseBindMatrices() const noexcept
    {
        return m_upImpl->vInverseBindMatrices;
    }

    const Skeleton::Impl& Skeleton::GetImpl() const noexcept
    {
        return *m_upImpl;
    }

    void Skeleton::RequireJoint(uint32_t uJoint) const
    {
        if (uJoint >= GetJointCount())
        {
            throw std::out_of_range(std::format("Joint index {} is out of range for skeleton '{}' with {} joint(s)",
                                                uJoint,
                                                m_upImpl->sName,
                                                GetJointCount()));
        }
    }
}
