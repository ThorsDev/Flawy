#pragma once

#include "Flawy/Assets/AssetId.hpp"
#include "Flawy/Assets/AssetTypeId.hpp"

#include <string>

namespace Flawy::Assets
{
    class Asset
    {
    public:
        virtual ~Asset() = default;

        Asset(const Asset&) = delete;
        Asset& operator=(const Asset&) = delete;
        Asset(Asset&&) = delete;
        Asset& operator=(Asset&&) = delete;

        [[nodiscard]] virtual AssetTypeId GetTypeId() const noexcept = 0;
        [[nodiscard]] const AssetId& GetId() const noexcept;
        [[nodiscard]] const std::string& GetName() const noexcept;

    protected:
        Asset(AssetId oId, std::string sName);

    private:
        AssetId m_oId;
        std::string m_sName;
    };
}
