#include "Flawy/Animation/AnimationControllerAsset.hpp"

#include <utility>

namespace Flawy::Animation
{
    AnimationControllerAsset::AnimationControllerAsset(Assets::AssetId oId, const AnimationControllerDescriptor& oDescriptor)
        : Asset(std::move(oId), oDescriptor.sName)
        , m_oController(oDescriptor)
    {
    }

    Assets::AssetTypeId AnimationControllerAsset::GetTypeId() const noexcept
    {
        return kTypeId;
    }

    const AnimationController& AnimationControllerAsset::GetController() const noexcept
    {
        return m_oController;
    }
}
