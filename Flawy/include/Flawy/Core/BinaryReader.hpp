#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace Flawy::Core
{
    class BinaryReader final
    {
    public:
        explicit BinaryReader(std::span<const std::byte> spanBytes, size_t uBaseOffset = 0) noexcept;

        [[nodiscard]] uint8_t ReadU8();
        [[nodiscard]] uint16_t ReadU16();
        [[nodiscard]] uint32_t ReadU32();
        [[nodiscard]] uint64_t ReadU64();
        [[nodiscard]] int32_t ReadI32();
        [[nodiscard]] float ReadF32();
        [[nodiscard]] bool ReadBool();
        [[nodiscard]] std::string ReadString(uint32_t uMaxLength);
        [[nodiscard]] std::span<const std::byte> ReadBytes(size_t uCount);
        void ReadF32Array(std::vector<float>& vOut, uint32_t uMaxCount);
        void ReadU32Array(std::vector<uint32_t>& vOut, uint32_t uMaxCount);
        void RequireElements(size_t uCount, size_t uElementSize) const;

        [[nodiscard]] size_t GetPosition() const noexcept;
        [[nodiscard]] size_t GetOffset() const noexcept;
        [[nodiscard]] size_t GetRemaining() const noexcept;
        [[nodiscard]] bool IsAtEnd() const noexcept;

    private:
        void Require(size_t uCount) const;
        [[nodiscard]] uint32_t ReadCount(uint32_t uMaxCount, size_t uElementSize, const char* szWhat);

        std::span<const std::byte> m_spanBytes;
        size_t m_uBaseOffset;
        size_t m_uPosition = 0;
    };
}
