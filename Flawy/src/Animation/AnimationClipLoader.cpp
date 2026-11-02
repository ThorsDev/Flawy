#include "Animation/AnimationClipLoader.hpp"

#include "Flawy/Animation/AnimationClipAsset.hpp"
#include "Flawy/Animation/AnimationClipDescriptorSerializer.hpp"
#include "Flawy/Assets/AssetManager.hpp"

#include <array>
#include <format>
#include <stdexcept>
#include <utility>

namespace Flawy::Animation
{
    namespace
    {
        constexpr std::array<std::string_view, 1> kExtensions = {AnimationClipAsset::kExtension};
    }

    Assets::AssetTypeId AnimationClipLoader::GetTypeId() const noexcept
    {
        return AnimationClipAsset::kTypeId;
    }

    std::string_view AnimationClipLoader::GetTypeName() const noexcept
    {
        return AnimationClipAsset::kTypeName;
    }

    std::span<const std::string_view> AnimationClipLoader::GetExtensions() const noexcept
    {
        return kExtensions;
    }

    std::shared_ptr<Assets::Asset> AnimationClipLoader::Load(const Assets::AssetLoadContext& oContext)
    {
        const AnimationClipDescriptor oDescriptor = AnimationClipDescriptorSerializer::Decode(oContext.spanBytes);
        if (oDescriptor.sSkeletonPath.empty())
        {
            throw std::invalid_argument(std::format("Animation clip asset '{}' does not reference a skeleton asset",
                                                    oContext.oId.GetPath()));
        }

        Assets::AssetHandle<SkeletonAsset> oSkeleton = oContext.oManager.Load<SkeletonAsset>(oDescriptor.sSkeletonPath);
        return std::make_shared<AnimationClipAsset>(oContext.oId, std::move(oSkeleton), oDescriptor);
    }
}
