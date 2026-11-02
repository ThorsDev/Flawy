#pragma once

#include <cstdint>
#include <string>
#include <string_view>

namespace Flawy::Assets
{
    class AssetId final
    {
    public:
        AssetId() = default;

        [[nodiscard]] static AssetId FromPath(std::string_view svNormalizedRelativePath);

        [[nodiscard]] uint64_t GetHash() const noexcept;
        [[nodiscard]] const std::string& GetPath() const noexcept;
        [[nodiscard]] bool IsValid() const noexcept;
        [[nodiscard]] bool operator==(const AssetId& oOther) const noexcept;

    private:
        uint64_t m_uHash = 0;
        std::string m_sPath;
    };
}
