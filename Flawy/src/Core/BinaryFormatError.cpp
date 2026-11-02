#include "Flawy/Core/BinaryFormatError.hpp"

#include <format>

namespace Flawy::Core
{
    BinaryFormatError::BinaryFormatError(std::string_view svMessage, size_t uOffset)
        : std::runtime_error(std::format("{} (at byte offset {})", svMessage, uOffset))
        , m_uOffset(uOffset)
    {
    }

    size_t BinaryFormatError::GetOffset() const noexcept
    {
        return m_uOffset;
    }
}
