#pragma once

#include "Flawy/Assets/AssetLoadContext.hpp"
#include "Flawy/Assets/AssetTypeId.hpp"

#include <memory>
#include <span>
#include <string_view>

namespace Flawy::Assets
{
    class Asset;

    class IAssetLoader
    {
    public:
        virtual ~IAssetLoader() = default;

        [[nodiscard]] virtual AssetTypeId GetTypeId() const noexcept = 0;
        [[nodiscard]] virtual std::string_view GetTypeName() const noexcept = 0;
        [[nodiscard]] virtual std::span<const std::string_view> GetExtensions() const noexcept = 0;
        [[nodiscard]] virtual std::shared_ptr<Asset> Load(const AssetLoadContext& oContext) = 0;
    };
}
