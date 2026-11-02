#include "Import/GltfAccessorReader.hpp"

#include <cstdint>
#include <optional>
#include <variant>

namespace Flawy::Import
{
    namespace
    {
        bool FitsInRange(size_t uOffset, size_t uLength, size_t uRangeSize) noexcept
        {
            return uLength <= uRangeSize && uOffset <= uRangeSize - uLength;
        }

        bool IsSparseIndexType(fastgltf::ComponentType eType) noexcept
        {
            return eType == fastgltf::ComponentType::UnsignedByte || eType == fastgltf::ComponentType::UnsignedShort ||
                   eType == fastgltf::ComponentType::UnsignedInt;
        }

        uint32_t ReadLittleEndian(std::span<const std::byte> spanBytes) noexcept
        {
            uint32_t uValue = 0;
            for (size_t uByte = 0; uByte < spanBytes.size(); ++uByte)
            {
                uValue |= static_cast<uint32_t>(spanBytes[uByte]) << (8u * uByte);
            }
            return uValue;
        }
    }

    GltfAccessorReader::GltfAccessorReader(const fastgltf::Asset& oAsset) noexcept
        : m_oAsset(oAsset)
    {
    }

    void GltfAccessorReader::ValidateStorage() const
    {
        ValidateBuffers();
        ValidateBufferViews();
        ValidateSparseAccessors();
    }

    fastgltf::AccessorType GltfAccessorReader::GetType(size_t uAccessor, std::string_view svUsage) const
    {
        if (uAccessor >= m_oAsset.accessors.size())
        {
            throw std::runtime_error(std::format("{} references missing accessor {}", svUsage, uAccessor));
        }
        return m_oAsset.accessors[uAccessor].type;
    }

    void GltfAccessorReader::ValidateBuffers() const
    {
        for (size_t uBuffer = 0; uBuffer < m_oAsset.buffers.size(); ++uBuffer)
        {
            const size_t uLoaded = GetLoadedBytes(uBuffer).size();
            const size_t uDeclared = m_oAsset.buffers[uBuffer].byteLength;
            if (uLoaded < uDeclared)
            {
                throw std::runtime_error(std::format("buffer {} declares {} byte(s) but only {} byte(s) were loaded", uBuffer, uDeclared, uLoaded));
            }
        }
    }

    void GltfAccessorReader::ValidateBufferViews() const
    {
        for (size_t uView = 0; uView < m_oAsset.bufferViews.size(); ++uView)
        {
            const fastgltf::BufferView& oView = m_oAsset.bufferViews[uView];
            if (oView.meshoptCompression)
            {
                throw std::runtime_error(std::format("buffer view {} uses meshopt compression, which is not supported", uView));
            }
            if (oView.bufferIndex >= m_oAsset.buffers.size())
            {
                throw std::runtime_error(std::format("buffer view {} references missing buffer {}", uView, oView.bufferIndex));
            }
            if (!FitsInRange(oView.byteOffset, oView.byteLength, m_oAsset.buffers[oView.bufferIndex].byteLength))
            {
                throw std::runtime_error(std::format("buffer view {} range [{}, +{}) exceeds buffer {}", uView, oView.byteOffset, oView.byteLength, oView.bufferIndex));
            }
        }
    }

    void GltfAccessorReader::ValidateSparseAccessors() const
    {
        for (size_t uAccessor = 0; uAccessor < m_oAsset.accessors.size(); ++uAccessor)
        {
            const fastgltf::Accessor& oAccessor = m_oAsset.accessors[uAccessor];
            if (!oAccessor.sparse)
            {
                continue;
            }

            const fastgltf::SparseAccessor& oSparse = *oAccessor.sparse;
            if (oSparse.indicesBufferView >= m_oAsset.bufferViews.size() || oSparse.valuesBufferView >= m_oAsset.bufferViews.size())
            {
                throw std::runtime_error(std::format("sparse accessor {} references a missing buffer view", uAccessor));
            }
            if (!IsSparseIndexType(oSparse.indexComponentType))
            {
                throw std::runtime_error(std::format("sparse accessor {} has an invalid index component type", uAccessor));
            }
            if (oSparse.count > oAccessor.count || oSparse.count > kMaxElements)
            {
                throw std::runtime_error(std::format("sparse accessor {} has {} sparse element(s) for {} element(s)", uAccessor, oSparse.count, oAccessor.count));
            }

            const size_t uIndexSize = fastgltf::getComponentByteSize(oSparse.indexComponentType);
            const size_t uElementSize = fastgltf::getElementByteSize(oAccessor.type, oAccessor.componentType);
            if (uElementSize == 0)
            {
                throw std::runtime_error(std::format("sparse accessor {} has an invalid element type", uAccessor));
            }
            if (!FitsInRange(oSparse.indicesByteOffset, oSparse.count * uIndexSize, m_oAsset.bufferViews[oSparse.indicesBufferView].byteLength))
            {
                throw std::runtime_error(std::format("sparse accessor {} indices exceed buffer view {}", uAccessor, oSparse.indicesBufferView));
            }
            if (!FitsInRange(oSparse.valuesByteOffset, oSparse.count * uElementSize, m_oAsset.bufferViews[oSparse.valuesBufferView].byteLength))
            {
                throw std::runtime_error(std::format("sparse accessor {} values exceed buffer view {}", uAccessor, oSparse.valuesBufferView));
            }
        }
    }

    const fastgltf::Accessor& GltfAccessorReader::RequireReadable(size_t uAccessor, std::string_view svUsage) const
    {
        if (uAccessor >= m_oAsset.accessors.size())
        {
            throw std::runtime_error(std::format("{} references missing accessor {}", svUsage, uAccessor));
        }
        const fastgltf::Accessor& oAccessor = m_oAsset.accessors[uAccessor];
        if (oAccessor.count > kMaxElements)
        {
            throw std::runtime_error(std::format("{} accessor {} has {} elements, the maximum is {}", svUsage, uAccessor, oAccessor.count, kMaxElements));
        }
        if (oAccessor.componentType == fastgltf::ComponentType::Double || oAccessor.componentType == fastgltf::ComponentType::Invalid)
        {
            throw std::runtime_error(std::format("{} accessor {} has an unsupported component type", svUsage, uAccessor));
        }
        if (oAccessor.sparse)
        {
            ValidateSparseIndices(oAccessor, uAccessor, svUsage);
        }
        return oAccessor;
    }

    void GltfAccessorReader::ValidateSparseIndices(const fastgltf::Accessor& oAccessor, size_t uAccessor, std::string_view svUsage) const
    {
        const fastgltf::SparseAccessor& oSparse = *oAccessor.sparse;
        const size_t uIndexSize = fastgltf::getComponentByteSize(oSparse.indexComponentType);
        const std::span<const std::byte> spanIndices = GetViewBytes(oSparse.indicesBufferView).subspan(oSparse.indicesByteOffset, oSparse.count * uIndexSize);

        std::optional<uint32_t> optPrevious;
        for (size_t uEntry = 0; uEntry < oSparse.count; ++uEntry)
        {
            const uint32_t uIndex = ReadLittleEndian(spanIndices.subspan(uEntry * uIndexSize, uIndexSize));
            if (uIndex >= oAccessor.count || (optPrevious && uIndex <= *optPrevious))
            {
                throw std::runtime_error(std::format("{} sparse accessor {} index {} is out of range or not strictly increasing", svUsage, uAccessor, uIndex));
            }
            optPrevious = uIndex;
        }
    }

    std::span<const std::byte> GltfAccessorReader::GetLoadedBytes(size_t uBuffer) const
    {
        const fastgltf::DataSource& oData = m_oAsset.buffers[uBuffer].data;
        if (const auto* pArray = std::get_if<fastgltf::sources::Array>(&oData))
        {
            return {pArray->bytes.data(), pArray->bytes.size_bytes()};
        }
        if (const auto* pVector = std::get_if<fastgltf::sources::Vector>(&oData))
        {
            return {pVector->bytes.data(), pVector->bytes.size()};
        }
        if (const auto* pView = std::get_if<fastgltf::sources::ByteView>(&oData))
        {
            return {pView->bytes.data(), pView->bytes.size()};
        }
        throw std::runtime_error(std::format("buffer {} has no loaded data", uBuffer));
    }

    std::span<const std::byte> GltfAccessorReader::GetViewBytes(size_t uView) const
    {
        const fastgltf::BufferView& oView = m_oAsset.bufferViews[uView];
        return GetLoadedBytes(oView.bufferIndex).subspan(oView.byteOffset, oView.byteLength);
    }
}
