#include "Flawy/Animation/SkeletonAsset.hpp"

#include "Flawy/Animation/Skeleton.hpp"
#include "Flawy/Animation/SkeletonDescriptor.hpp"

#include <utility>

namespace Flawy::Animation
{
    SkeletonAsset::SkeletonAsset(Assets::AssetId oId, const SkeletonDescriptor& oDescriptor)
        : Asset(std::move(oId), oDescriptor.sName)
        , m_upSkeleton(std::make_unique<Skeleton>(oDescriptor))
    {
    }

    SkeletonAsset::~SkeletonAsset() = default;

    Assets::AssetTypeId SkeletonAsset::GetTypeId() const noexcept
    {
        return kTypeId;
    }

    const Skeleton& SkeletonAsset::GetSkeleton() const noexcept
    {
        return *m_upSkeleton;
    }
}
