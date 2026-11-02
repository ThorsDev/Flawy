#pragma once

#include <cstddef>
#include <stdexcept>
#include <string_view>

namespace Flawy::Core
{
    class BinaryFormatError final : public std::runtime_error
    {
    public:
        BinaryFormatError(std::string_view svMessage, size_t uOffset);

        [[nodiscard]] size_t GetOffset() const noexcept;

    private:
        size_t m_uOffset;
    };
}
