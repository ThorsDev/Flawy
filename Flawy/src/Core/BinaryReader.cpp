#include "Flawy/Core/BinaryReader.hpp"

#include "Flawy/Core/BinaryFormatError.hpp"

#include <bit>
#include <cmath>
#include <format>

namespace Flawy::Core
{
    BinaryReader::BinaryReader(std::span<const std::byte> spanBytes, size_t uBaseOffset) noexcept
        : m_spanBytes(spanBytes)
        , m_uBaseOffset(uBaseOffset)
    {
    }

    uint8_t BinaryReader::ReadU8()
    {
        Require(1);
        const auto uValue = static_cast<uint8_t>(m_spanBytes[m_uPosition]);
        ++m_uPosition;
        return uValue;
    }

    uint16_t BinaryReader::ReadU16()
    {
        Require(2);
        const auto uLow = static_cast<uint16_t>(ReadU8());
        const auto uHigh = static_cast<uint16_t>(ReadU8());
        return static_cast<uint16_t>(uLow | static_cast<uint16_t>(uHigh << 8));
    }

    uint32_t BinaryReader::ReadU32()
    {
        Require(4);
        uint32_t uValue = 0;
        for (uint32_t uShift = 0; uShift < 32; uShift += 8)
        {
            uValue |= static_cast<uint32_t>(ReadU8()) << uShift;
        }
        return uValue;
    }

    uint64_t BinaryReader::ReadU64()
    {
        Require(8);
        uint64_t uValue = 0;
        for (uint32_t uShift = 0; uShift < 64; uShift += 8)
        {
            uValue |= static_cast<uint64_t>(ReadU8()) << uShift;
        }
        return uValue;
    }

    int32_t BinaryReader::ReadI32()
    {
        return std::bit_cast<int32_t>(ReadU32());
    }

    float BinaryReader::ReadF32()
    {
        const size_t uOffset = GetOffset();
        const float fValue = std::bit_cast<float>(ReadU32());
        if (!std::isfinite(fValue))
        {
            throw BinaryFormatError("non-finite floating point value", uOffset);
        }
        return fValue;
    }

    bool BinaryReader::ReadBool()
    {
        const size_t uOffset = GetOffset();
        const uint8_t uValue = ReadU8();
        if (uValue > 1)
        {
            throw BinaryFormatError(std::format("invalid boolean value {}", uValue), uOffset);
        }
        return uValue == 1;
    }

    std::string BinaryReader::ReadString(uint32_t uMaxLength)
    {
        const uint32_t uLength = ReadCount(uMaxLength, 1, "string length");
        const std::span<const std::byte> spanText = ReadBytes(uLength);
        std::string sText(uLength, '\0');
        for (size_t uIndex = 0; uIndex < spanText.size(); ++uIndex)
        {
            sText[uIndex] = static_cast<char>(spanText[uIndex]);
        }
        return sText;
    }

    std::span<const std::byte> BinaryReader::ReadBytes(size_t uCount)
    {
        Require(uCount);
        const std::span<const std::byte> spanResult = m_spanBytes.subspan(m_uPosition, uCount);
        m_uPosition += uCount;
        return spanResult;
    }

    void BinaryReader::ReadF32Array(std::vector<float>& vOut, uint32_t uMaxCount)
    {
        const uint32_t uCount = ReadCount(uMaxCount, sizeof(float), "float array length");
        vOut.clear();
        vOut.reserve(uCount);
        for (uint32_t uIndex = 0; uIndex < uCount; ++uIndex)
        {
            vOut.push_back(ReadF32());
        }
    }

    void BinaryReader::ReadU32Array(std::vector<uint32_t>& vOut, uint32_t uMaxCount)
    {
        const uint32_t uCount = ReadCount(uMaxCount, sizeof(uint32_t), "uint32 array length");
        vOut.clear();
        vOut.reserve(uCount);
        for (uint32_t uIndex = 0; uIndex < uCount; ++uIndex)
        {
            vOut.push_back(ReadU32());
        }
    }

    void BinaryReader::RequireElements(size_t uCount, size_t uElementSize) const
    {
        if (uElementSize != 0 && uCount > GetRemaining() / uElementSize)
        {
            throw BinaryFormatError(std::format("{} element(s) of {} byte(s) exceed the {} remaining byte(s)", uCount, uElementSize, GetRemaining()), GetOffset());
        }
    }

    size_t BinaryReader::GetPosition() const noexcept
    {
        return m_uPosition;
    }

    size_t BinaryReader::GetOffset() const noexcept
    {
        return m_uBaseOffset + m_uPosition;
    }

    size_t BinaryReader::GetRemaining() const noexcept
    {
        return m_spanBytes.size() - m_uPosition;
    }

    bool BinaryReader::IsAtEnd() const noexcept
    {
        return m_uPosition == m_spanBytes.size();
    }

    void BinaryReader::Require(size_t uCount) const
    {
        if (uCount > GetRemaining())
        {
            throw BinaryFormatError(std::format("unexpected end of data reading {} byte(s)", uCount), GetOffset());
        }
    }

    uint32_t BinaryReader::ReadCount(uint32_t uMaxCount, size_t uElementSize, const char* szWhat)
    {
        const size_t uOffset = GetOffset();
        const uint32_t uCount = ReadU32();
        if (uCount > uMaxCount)
        {
            throw BinaryFormatError(std::format("{} {} exceeds the maximum of {}", szWhat, uCount, uMaxCount), uOffset);
        }
        RequireElements(uCount, uElementSize);
        return uCount;
    }
}
