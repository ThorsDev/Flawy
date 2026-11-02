#pragma once

#include "Flawy/Assets/AssetId.hpp"

#include <memory>
#include <stdexcept>
#include <utility>

namespace Flawy::Assets
{
    template <typename TAsset>
    class AssetHandle final
    {
    public:
        AssetHandle() = default;

        explicit AssetHandle(std::shared_ptr<TAsset> spAsset) noexcept
            : m_spAsset(std::move(spAsset))
        {
        }

        [[nodiscard]] bool IsValid() const noexcept
        {
            return m_spAsset != nullptr;
        }

        [[nodiscard]] TAsset& Get() const
        {
            RequireValid();
            return *m_spAsset;
        }

        [[nodiscard]] TAsset* operator->() const
        {
            RequireValid();
            return m_spAsset.get();
        }

        [[nodiscard]] const AssetId& GetId() const
        {
            RequireValid();
            return m_spAsset->GetId();
        }

        void Reset() noexcept
        {
            m_spAsset.reset();
        }

    private:
        void RequireValid() const
        {
            if (!m_spAsset)
            {
                throw std::logic_error("AssetHandle is empty");
            }
        }

        std::shared_ptr<TAsset> m_spAsset;
    };
}
