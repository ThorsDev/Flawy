#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace Flawy::Renderer
{
    class MaterialText final
    {
    public:
        MaterialText() = delete;

        [[nodiscard]] static constexpr bool IsAsciiDigit(char cCharacter) noexcept
        {
            return cCharacter >= '0' && cCharacter <= '9';
        }

        [[nodiscard]] static constexpr bool IsAsciiLetter(char cCharacter) noexcept
        {
            return (cCharacter >= 'a' && cCharacter <= 'z') || (cCharacter >= 'A' && cCharacter <= 'Z');
        }

        [[nodiscard]] static constexpr bool IsIdentifierCharacter(char cCharacter) noexcept
        {
            return IsAsciiLetter(cCharacter) || IsAsciiDigit(cCharacter) || cCharacter == '_';
        }

        [[nodiscard]] static constexpr bool IsIdentifier(std::string_view svText) noexcept
        {
            if (svText.empty() || IsAsciiDigit(svText.front()))
            {
                return false;
            }
            for (const char cCharacter : svText)
            {
                if (!IsIdentifierCharacter(cCharacter))
                {
                    return false;
                }
            }
            return true;
        }

        [[nodiscard]] static constexpr bool IsValidUtf8(std::string_view svText) noexcept
        {
            size_t uIndex = 0;
            while (uIndex < svText.size())
            {
                const auto uLead = static_cast<uint8_t>(svText[uIndex]);
                size_t uLength = 0;
                uint32_t uCodePoint = 0;
                if (uLead < 0x80u)
                {
                    ++uIndex;
                    continue;
                }
                if ((uLead & 0xe0u) == 0xc0u)
                {
                    uLength = 2;
                    uCodePoint = uLead & 0x1fu;
                }
                else if ((uLead & 0xf0u) == 0xe0u)
                {
                    uLength = 3;
                    uCodePoint = uLead & 0x0fu;
                }
                else if ((uLead & 0xf8u) == 0xf0u)
                {
                    uLength = 4;
                    uCodePoint = uLead & 0x07u;
                }
                else
                {
                    return false;
                }
                if (uIndex + uLength > svText.size())
                {
                    return false;
                }
                for (size_t uOffset = 1; uOffset < uLength; ++uOffset)
                {
                    const auto uContinuation = static_cast<uint8_t>(svText[uIndex + uOffset]);
                    if ((uContinuation & 0xc0u) != 0x80u)
                    {
                        return false;
                    }
                    uCodePoint = (uCodePoint << 6) | (uContinuation & 0x3fu);
                }
                const bool bOverlong = (uLength == 2 && uCodePoint < 0x80u) || (uLength == 3 && uCodePoint < 0x800u) || (uLength == 4 && uCodePoint < 0x10000u);
                const bool bSurrogate = uCodePoint >= 0xd800u && uCodePoint <= 0xdfffu;
                if (bOverlong || bSurrogate || uCodePoint > 0x10ffffu)
                {
                    return false;
                }
                uIndex += uLength;
            }
            return true;
        }
    };
}
