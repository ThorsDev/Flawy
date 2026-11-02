#include "Animation/SkeletonLoader.hpp"

#include "Flawy/Animation/SkeletonAsset.hpp"
#include "Flawy/Animation/SkeletonDescriptorSerializer.hpp"

#include <array>

namespace Flawy::Animation
{
    namespace
    {
        constexpr std::array<std::string_view, 1> kExtensions = {SkeletonAsset::kExtension};
    }

    Assets::AssetTypeId SkeletonLoader::GetTypeId() const noexcept
    {
        return SkeletonAsset::kTypeId;
    }

    std::string_view SkeletonLoader::GetTypeName() const noexcept
    {
        return SkeletonAsset::kTypeName;
    }

    std::span<const std::string_view> SkeletonLoader::GetExtensions() const noexcept
    {
        return kExtensions;
    }

    std::shared_ptr<Assets::Asset> SkeletonLoader::Load(const Assets::AssetLoadContext& oContext)
    {
        SkeletonDescriptor oDescriptor = SkeletonDescriptorSerializer::Decode(oContext.spanBytes);
        if (oDescriptor.sName.empty())
        {
            oDescriptor.sName = oContext.sName;
        }
        return std::make_shared<SkeletonAsset>(oContext.oId, oDescriptor);
    }
}
