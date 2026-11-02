#include "Animation/AnimationControllerLoader.hpp"

#include "Flawy/Animation/AnimationControllerAsset.hpp"
#include "Flawy/Animation/AnimationControllerDescriptor.hpp"

#include <array>
#include <string_view>

namespace Flawy::Animation
{
    namespace
    {
        constexpr std::array<std::string_view, 1> kExtensions = {AnimationControllerAsset::kExtension};
    }

    Assets::AssetTypeId AnimationControllerLoader::GetTypeId() const noexcept
    {
        return AnimationControllerAsset::kTypeId;
    }

    std::string_view AnimationControllerLoader::GetTypeName() const noexcept
    {
        return AnimationControllerAsset::kTypeName;
    }

    std::span<const std::string_view> AnimationControllerLoader::GetExtensions() const noexcept
    {
        return kExtensions;
    }

    std::shared_ptr<Assets::Asset> AnimationControllerLoader::Load(const Assets::AssetLoadContext& oContext)
    {
        const std::string_view svJson(reinterpret_cast<const char*>(oContext.spanBytes.data()), oContext.spanBytes.size());
        const AnimationControllerDescriptor oDescriptor = AnimationControllerDescriptor::FromJson(svJson);
        return std::make_shared<AnimationControllerAsset>(oContext.oId, oDescriptor);
    }
}
