#include "Assets/StaticMeshLoader.hpp"

#include "Flawy/Assets/MeshDataSerializer.hpp"
#include "Flawy/Assets/StaticMeshAsset.hpp"

#include <array>
#include <utility>

namespace Flawy::Assets
{
    namespace
    {
        constexpr std::array<std::string_view, 1> kExtensions = {StaticMeshAsset::kExtension};
    }

    AssetTypeId StaticMeshLoader::GetTypeId() const noexcept
    {
        return StaticMeshAsset::kTypeId;
    }

    std::string_view StaticMeshLoader::GetTypeName() const noexcept
    {
        return StaticMeshAsset::kTypeName;
    }

    std::span<const std::string_view> StaticMeshLoader::GetExtensions() const noexcept
    {
        return kExtensions;
    }

    std::shared_ptr<Asset> StaticMeshLoader::Load(const AssetLoadContext& oContext)
    {
        MeshData oMeshData = MeshDataSerializer::Decode(oContext.spanBytes);
        if (oMeshData.sName.empty())
        {
            oMeshData.sName = oContext.sName;
        }
        return std::make_shared<StaticMeshAsset>(oContext.oId, std::move(oMeshData));
    }
}
