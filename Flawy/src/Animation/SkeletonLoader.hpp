#pragma once

#include "Flawy/Assets/IAssetLoader.hpp"

namespace Flawy::Animation
{
    class SkeletonLoader final : public Assets::IAssetLoader
    {
    public:
        [[nodiscard]] Assets::AssetTypeId GetTypeId() const noexcept override;
        [[nodiscard]] std::string_view GetTypeName() const noexcept override;
        [[nodiscard]] std::span<const std::string_view> GetExtensions() const noexcept override;
        [[nodiscard]] std::shared_ptr<Assets::Asset> Load(const Assets::AssetLoadContext& oContext) override;
    };
}
