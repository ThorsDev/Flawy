#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <vector>

namespace Flawy::Core
{
    class BinaryWriter final
    {
    public:
        void WriteU8(uint8_t uValue);
        void WriteU16(uint16_t uValue);
        void WriteU32(uint32_t uValue);
        void WriteU64(uint64_t uValue);
        void WriteI32(int32_t iValue);
        void WriteF32(float fValue);
        void WriteBool(bool bValue);
        void WriteString(std::string_view svText);
        void WriteBytes(std::span<const std::byte> spanBytes);
        void WriteF32Array(std::span<const float> spanValues);
        void WriteU32Array(std::span<const uint32_t> spanValues);

        [[nodiscard]] std::span<const std::byte> GetBytes() const noexcept;
        [[nodiscard]] size_t GetSize() const noexcept;
        [[nodiscard]] std::vector<std::byte> TakeBytes() noexcept;

    private:
        void WriteCount(size_t uCount);

        std::vector<std::byte> m_vBytes;
    };
}
