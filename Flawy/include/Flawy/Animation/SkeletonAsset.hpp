#pragma once

#include "Flawy/Assets/Asset.hpp"

#include <memory>
#include <string_view>

namespace Flawy::Animation
{
    class Skeleton;
    struct SkeletonDescriptor;

    class SkeletonAsset final : public Assets::Asset
    {
    public:
        static constexpr std::string_view kTypeName = "Skeleton";
        static constexpr Assets::AssetTypeId kTypeId = Assets::AssetTypeId::FromName(kTypeName);
        static constexpr std::string_view kExtension = ".flskel";

        SkeletonAsset(Assets::AssetId oId, const SkeletonDescriptor& oDescriptor);
        ~SkeletonAsset() override;

        [[nodiscard]] Assets::AssetTypeId GetTypeId() const noexcept override;
        [[nodiscard]] const Skeleton& GetSkeleton() const noexcept;

    private:
        std::unique_ptr<Skeleton> m_upSkeleton;
    };
}
