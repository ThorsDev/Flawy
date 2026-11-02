#include "Flawy/Animation/AnimationClipAsset.hpp"

#include "Flawy/Animation/AnimationClip.hpp"
#include "Flawy/Animation/AnimationClipDescriptor.hpp"
#include "Flawy/Animation/Skeleton.hpp"

#include <format>
#include <stdexcept>
#include <utility>

namespace Flawy::Animation
{
    namespace
    {
        const Skeleton& RequireSkeleton(const Assets::AssetHandle<SkeletonAsset>& oSkeleton, const AnimationClipDescriptor& oDescriptor)
        {
            if (!oSkeleton.IsValid())
            {
                throw std::invalid_argument(std::format("Animation clip asset '{}' needs a skeleton asset", oDescriptor.sName));
            }
            return oSkeleton->GetSkeleton();
        }
    }

    AnimationClipAsset::AnimationClipAsset(Assets::AssetId oId,
                                           Assets::AssetHandle<SkeletonAsset> oSkeleton,
                                           const AnimationClipDescriptor& oDescriptor)
        : Asset(std::move(oId), oDescriptor.sName)
        , m_oSkeleton(std::move(oSkeleton))
        , m_upClip(std::make_unique<AnimationClip>(RequireSkeleton(m_oSkeleton, oDescriptor), oDescriptor))
    {
    }

    AnimationClipAsset::~AnimationClipAsset() = default;

    Assets::AssetTypeId AnimationClipAsset::GetTypeId() const noexcept
    {
        return kTypeId;
    }

    const AnimationClip& AnimationClipAsset::GetClip() const noexcept
    {
        return *m_upClip;
    }

    const Assets::AssetHandle<SkeletonAsset>& AnimationClipAsset::GetSkeletonAsset() const noexcept
    {
        return m_oSkeleton;
    }
}
