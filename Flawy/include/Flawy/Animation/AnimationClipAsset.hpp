#pragma once

#include "Flawy/Animation/SkeletonAsset.hpp"
#include "Flawy/Assets/Asset.hpp"
#include "Flawy/Assets/AssetHandle.hpp"

#include <memory>
#include <string_view>

namespace Flawy::Animation
{
    class AnimationClip;
    struct AnimationClipDescriptor;

    class AnimationClipAsset final : public Assets::Asset
    {
    public:
        static constexpr std::string_view kTypeName = "AnimationClip";
        static constexpr Assets::AssetTypeId kTypeId = Assets::AssetTypeId::FromName(kTypeName);
        static constexpr std::string_view kExtension = ".flclip";

        AnimationClipAsset(Assets::AssetId oId, Assets::AssetHandle<SkeletonAsset> oSkeleton, const AnimationClipDescriptor& oDescriptor);
        ~AnimationClipAsset() override;

        [[nodiscard]] Assets::AssetTypeId GetTypeId() const noexcept override;
        [[nodiscard]] const AnimationClip& GetClip() const noexcept;
        [[nodiscard]] const Assets::AssetHandle<SkeletonAsset>& GetSkeletonAsset() const noexcept;

    private:
        Assets::AssetHandle<SkeletonAsset> m_oSkeleton;
        std::unique_ptr<AnimationClip> m_upClip;
    };
}
