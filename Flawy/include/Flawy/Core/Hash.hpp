#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>
#include <type_traits>

namespace Flawy::Core
{
    class Fnv1aHasher final
    {
    public:
        static constexpr uint64_t kDefaultSeed = 0xcbf29ce484222325ull;
        static constexpr uint64_t kAlternateSeed = 0x84222325cbf29ce4ull;

        explicit Fnv1aHasher(uint64_t uSeed = kDefaultSeed) noexcept;

        void Append(std::span<const std::byte> spanBytes) noexcept;
        void Append(std::string_view svText) noexcept;

        template <typename TValue>
            requires std::is_trivially_copyable_v<TValue>
        void AppendValue(const TValue& oValue) noexcept
        {
            Append(std::as_bytes(std::span<const TValue, 1>(&oValue, 1)));
        }

        [[nodiscard]] uint64_t GetValue() const noexcept;

        [[nodiscard]] static constexpr uint64_t HashText(std::string_view svText, uint64_t uSeed = kDefaultSeed) noexcept
        {
            uint64_t uState = uSeed;
            for (const char cCharacter : svText)
            {
                uState ^= static_cast<uint64_t>(static_cast<unsigned char>(cCharacter));
                uState *= kPrime;
            }
            return uState;
        }

    private:
        static constexpr uint64_t kPrime = 0x100000001b3ull;

        uint64_t m_uState;
    };
}
