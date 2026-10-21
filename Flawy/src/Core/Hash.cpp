#include "Flawy/Core/Hash.hpp"

namespace Flawy::Core
{
    Fnv1aHasher::Fnv1aHasher(uint64_t uSeed) noexcept
        : m_uState(uSeed)
    {
    }

    void Fnv1aHasher::Append(std::span<const std::byte> spanBytes) noexcept
    {
        for (const std::byte uByte : spanBytes)
        {
            m_uState ^= static_cast<uint64_t>(uByte);
            m_uState *= kPrime;
        }
    }

    void Fnv1aHasher::Append(std::string_view svText) noexcept
    {
        AppendValue(static_cast<uint64_t>(svText.size()));
        Append(std::as_bytes(std::span<const char>(svText.data(), svText.size())));
    }

    uint64_t Fnv1aHasher::GetValue() const noexcept
    {
        return m_uState;
    }
}
