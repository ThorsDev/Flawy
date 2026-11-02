#include "Flawy/Animation/PoseBlender.hpp"

#include "Animation/Ozz/OzzLocalPoseImpl.hpp"
#include "Animation/Ozz/OzzSkeletonImpl.hpp"
#include "Flawy/Animation/LocalPose.hpp"
#include "Flawy/Animation/Skeleton.hpp"

#include <ozz/animation/runtime/blending_job.h>
#include <ozz/base/span.h>

#include <cmath>
#include <format>
#include <stdexcept>
#include <vector>

namespace Flawy::Animation
{
    struct PoseBlender::Impl
    {
        Impl(const Skeleton& oOwner, uint32_t uMaxLayers)
            : oSkeleton(oOwner)
            , vLayers(uMaxLayers)
        {
        }

        void ValidateLayer(const BlendLayer& oLayer, size_t uLayer, const LocalPose& oOutput) const
        {
            if (oLayer.pPose == nullptr)
            {
                throw std::invalid_argument(std::format("Blend layer {} has no pose", uLayer));
            }
            if (!std::isfinite(oLayer.fWeight) || oLayer.fWeight < 0.0f)
            {
                throw std::invalid_argument(std::format("Blend layer {} has weight {}; weights must be finite and non-negative",
                                                        uLayer,
                                                        oLayer.fWeight));
            }
            if (&oLayer.pPose->GetSkeleton() != &oSkeleton)
            {
                throw std::invalid_argument(std::format("Blend layer {} uses skeleton '{}' but the blender uses skeleton '{}'",
                                                        uLayer,
                                                        oLayer.pPose->GetSkeleton().GetName(),
                                                        oSkeleton.GetName()));
            }
            if (oLayer.pPose == &oOutput)
            {
                throw std::invalid_argument(std::format("Blend layer {} must not be the output pose", uLayer));
            }
        }

        const Skeleton& oSkeleton;
        std::vector<ozz::animation::BlendingJob::Layer> vLayers;
    };

    PoseBlender::PoseBlender(const Skeleton& oSkeleton, uint32_t uMaxLayers)
    {
        if (uMaxLayers == 0)
        {
            throw std::invalid_argument("Pose blender needs room for at least one layer");
        }
        m_upImpl = std::make_unique<Impl>(oSkeleton, uMaxLayers);
    }

    PoseBlender::~PoseBlender() = default;

    void PoseBlender::Blend(std::span<const BlendLayer> spanLayers, LocalPose& oOutput)
    {
        Impl& oImpl = *m_upImpl;
        if (spanLayers.empty() || spanLayers.size() > oImpl.vLayers.size())
        {
            throw std::invalid_argument(std::format("Pose blender received {} layer(s); it supports 1 to {}",
                                                    spanLayers.size(),
                                                    oImpl.vLayers.size()));
        }
        if (&oOutput.GetSkeleton() != &oImpl.oSkeleton)
        {
            throw std::invalid_argument(std::format("Pose blender of skeleton '{}' cannot write a pose of skeleton '{}'",
                                                    oImpl.oSkeleton.GetName(),
                                                    oOutput.GetSkeleton().GetName()));
        }

        for (size_t uLayer = 0; uLayer < spanLayers.size(); ++uLayer)
        {
            const BlendLayer& oLayer = spanLayers[uLayer];
            oImpl.ValidateLayer(oLayer, uLayer, oOutput);
            oImpl.vLayers[uLayer].weight = oLayer.fWeight;
            oImpl.vLayers[uLayer].transform = ozz::make_span(oLayer.pPose->GetImpl().vTransforms);
        }

        ozz::animation::BlendingJob oJob;
        oJob.layers = ozz::span<const ozz::animation::BlendingJob::Layer>(oImpl.vLayers.data(), spanLayers.size());
        oJob.rest_pose = oImpl.oSkeleton.GetImpl().upSkeleton->joint_rest_poses();
        oJob.output = ozz::make_span(oOutput.GetImpl().vTransforms);
        if (!oJob.Run())
        {
            throw std::logic_error(std::format("Blending job failed for skeleton '{}'", oImpl.oSkeleton.GetName()));
        }
    }

    uint32_t PoseBlender::GetMaxLayerCount() const noexcept
    {
        return static_cast<uint32_t>(m_upImpl->vLayers.size());
    }

    const Skeleton& PoseBlender::GetSkeleton() const noexcept
    {
        return m_upImpl->oSkeleton;
    }
}
