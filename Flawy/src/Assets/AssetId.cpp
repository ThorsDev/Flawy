#include "Flawy/Assets/AssetId.hpp"

#include "Flawy/Core/Hash.hpp"

namespace Flawy::Assets
{
    AssetId AssetId::FromPath(std::string_view svNormalizedRelativePath)
    {
        AssetId oId;
        oId.m_sPath = std::string(svNormalizedRelativePath);
        oId.m_uHash = oId.m_sPath.empty() ? 0 : Core::Fnv1aHasher::HashText(svNormalizedRelativePath);
        return oId;
    }

    uint64_t AssetId::GetHash() const noexcept
    {
        return m_uHash;
    }

    const std::string& AssetId::GetPath() const noexcept
    {
        return m_sPath;
    }

    bool AssetId::IsValid() const noexcept
    {
        return !m_sPath.empty();
    }

    bool AssetId::operator==(const AssetId& oOther) const noexcept
    {
        return m_uHash == oOther.m_uHash && m_sPath == oOther.m_sPath;
    }
}
