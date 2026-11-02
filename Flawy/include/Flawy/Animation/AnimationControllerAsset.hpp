#pragma once

#include "Flawy/Animation/AnimationController.hpp"
#include "Flawy/Assets/Asset.hpp"

#include <string_view>

namespace Flawy::Animation
{
    class AnimationControllerAsset final : public Assets::Asset
    {
    public:
        static constexpr std::string_view kTypeName = "AnimationController";
        static constexpr Assets::AssetTypeId kTypeId = Assets::AssetTypeId::FromName(kTypeName);
        static constexpr std::string_view kExtension = ".flctrl";

        AnimationControllerAsset(Assets::AssetId oId, const AnimationControllerDescriptor& oDescriptor);

        [[nodiscard]] Assets::AssetTypeId GetTypeId() const noexcept override;
        [[nodiscard]] const AnimationController& GetController() const noexcept;

    private:
        AnimationController m_oController;
    };
}
