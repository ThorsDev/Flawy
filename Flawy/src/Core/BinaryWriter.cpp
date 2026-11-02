#include "Flawy/Core/BinaryWriter.hpp"

#include <bit>
#include <limits>
#include <stdexcept>
#include <utility>

namespace Flawy::Core
{
    void BinaryWriter::WriteU8(uint8_t uValue)
    {
        m_vBytes.push_back(static_cast<std::byte>(uValue));
    }

    void BinaryWriter::WriteU16(uint16_t uValue)
    {
        WriteU8(static_cast<uint8_t>(uValue & 0xffu));
        WriteU8(static_cast<uint8_t>((uValue >> 8) & 0xffu));
    }

    void BinaryWriter::WriteU32(uint32_t uValue)
    {
        for (uint32_t uShift = 0; uShift < 32; uShift += 8)
        {
            WriteU8(static_cast<uint8_t>((uValue >> uShift) & 0xffu));
        }
    }

    void BinaryWriter::WriteU64(uint64_t uValue)
    {
        for (uint32_t uShift = 0; uShift < 64; uShift += 8)
        {
            WriteU8(static_cast<uint8_t>((uValue >> uShift) & 0xffu));
        }
    }

    void BinaryWriter::WriteI32(int32_t iValue)
    {
        WriteU32(std::bit_cast<uint32_t>(iValue));
    }

    void BinaryWriter::WriteF32(float fValue)
    {
        WriteU32(std::bit_cast<uint32_t>(fValue));
    }

    void BinaryWriter::WriteBool(bool bValue)
    {
        WriteU8(static_cast<uint8_t>(bValue ? 1 : 0));
    }

    void BinaryWriter::WriteString(std::string_view svText)
    {
        WriteCount(svText.size());
        WriteBytes(std::as_bytes(std::span<const char>(svText.data(), svText.size())));
    }

    void BinaryWriter::WriteBytes(std::span<const std::byte> spanBytes)
    {
        m_vBytes.insert(m_vBytes.end(), spanBytes.begin(), spanBytes.end());
    }

    void BinaryWriter::WriteF32Array(std::span<const float> spanValues)
    {
        WriteCount(spanValues.size());
        for (const float fValue : spanValues)
        {
            WriteF32(fValue);
        }
    }

    void BinaryWriter::WriteU32Array(std::span<const uint32_t> spanValues)
    {
        WriteCount(spanValues.size());
        for (const uint32_t uValue : spanValues)
        {
            WriteU32(uValue);
        }
    }

    std::span<const std::byte> BinaryWriter::GetBytes() const noexcept
    {
        return m_vBytes;
    }

    size_t BinaryWriter::GetSize() const noexcept
    {
        return m_vBytes.size();
    }

    std::vector<std::byte> BinaryWriter::TakeBytes() noexcept
    {
        return std::exchange(m_vBytes, {});
    }

    void BinaryWriter::WriteCount(size_t uCount)
    {
        if (uCount > std::numeric_limits<uint32_t>::max())
        {
            throw std::length_error("BinaryWriter cannot store more than 4294967295 elements in one block");
        }
        WriteU32(static_cast<uint32_t>(uCount));
    }
}
