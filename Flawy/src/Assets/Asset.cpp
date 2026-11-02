#include "Flawy/Assets/Asset.hpp"

#include <utility>

namespace Flawy::Assets
{
    Asset::Asset(AssetId oId, std::string sName)
        : m_oId(std::move(oId))
        , m_sName(std::move(sName))
    {
    }

    const AssetId& Asset::GetId() const noexcept
    {
        return m_oId;
    }

    const std::string& Asset::GetName() const noexcept
    {
        return m_sName;
    }
}
