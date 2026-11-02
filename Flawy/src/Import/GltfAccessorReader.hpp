#pragma once

#include "Flawy/Assets/MeshData.hpp"

#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>

#include <cstddef>
#include <format>
#include <span>
#include <stdexcept>
#include <string_view>
#include <vector>

namespace Flawy::Import
{
    class GltfAccessorReader final
    {
    public:
        static constexpr size_t kMaxElements = Assets::MeshData::kMaxIndexCount;

        explicit GltfAccessorReader(const fastgltf::Asset& oAsset) noexcept;

        void ValidateStorage() const;

        template <typename TElement>
        [[nodiscard]] std::vector<TElement> Read(size_t uAccessor, std::string_view svUsage) const
        {
            const fastgltf::Accessor& oAccessor = RequireReadable(uAccessor, svUsage);
            if (oAccessor.type != fastgltf::ElementTraits<TElement>::type)
            {
                throw std::runtime_error(std::format("{} accessor {} has an unexpected element type", svUsage, uAccessor));
            }

            std::vector<TElement> vValues(oAccessor.count);
            fastgltf::iterateAccessorWithIndex<TElement>(m_oAsset, oAccessor, [&vValues](TElement oValue, size_t uIndex) { vValues[uIndex] = oValue; });
            return vValues;
        }

        [[nodiscard]] fastgltf::AccessorType GetType(size_t uAccessor, std::string_view svUsage) const;

    private:
        void ValidateBuffers() const;
        void ValidateBufferViews() const;
        void ValidateSparseAccessors() const;
        [[nodiscard]] const fastgltf::Accessor& RequireReadable(size_t uAccessor, std::string_view svUsage) const;
        void ValidateSparseIndices(const fastgltf::Accessor& oAccessor, size_t uAccessor, std::string_view svUsage) const;
        [[nodiscard]] std::span<const std::byte> GetLoadedBytes(size_t uBuffer) const;
        [[nodiscard]] std::span<const std::byte> GetViewBytes(size_t uView) const;

        const fastgltf::Asset& m_oAsset;
    };
}
