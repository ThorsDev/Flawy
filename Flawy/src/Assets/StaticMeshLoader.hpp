#pragma once

#include "Flawy/Assets/IAssetLoader.hpp"

namespace Flawy::Assets
{
    class StaticMeshLoader final : public IAssetLoader
    {
    public:
        [[nodiscard]] AssetTypeId GetTypeId() const noexcept override;
        [[nodiscard]] std::string_view GetTypeName() const noexcept override;
        [[nodiscard]] std::span<const std::string_view> GetExtensions() const noexcept override;
        [[nodiscard]] std::shared_ptr<Asset> Load(const AssetLoadContext& oContext) override;
    };
}
