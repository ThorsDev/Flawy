#include "Renderer/Material/MaterialCustomCodeRules.hpp"

#include "Renderer/Material/MaterialText.hpp"

#include <array>
#include <format>
#include <vector>

namespace Flawy::Renderer
{
    namespace
    {
        constexpr std::array<std::string_view, 10> kForbiddenTokens = {";", "{", "}", "//", "/*", "#", "++", "--", "\n", "\r"};
        constexpr std::string_view kOutputIdentifier = "oOutput";

        std::string_view DescribeToken(std::string_view svToken) noexcept
        {
            if (svToken == "\n")
            {
                return "\\n";
            }
            if (svToken == "\r")
            {
                return "\\r";
            }
            return svToken;
        }

        bool IsAssignment(std::string_view svCode, size_t uPosition) noexcept
        {
            const char cPrevious = uPosition > 0 ? svCode[uPosition - 1] : '\0';
            const char cBeforePrevious = uPosition > 1 ? svCode[uPosition - 2] : '\0';
            const char cNext = uPosition + 1 < svCode.size() ? svCode[uPosition + 1] : '\0';
            const bool bRelational = (cPrevious == '<' || cPrevious == '>') && cBeforePrevious != cPrevious;
            const bool bComparisonTail = cPrevious == '=' || cPrevious == '!' || bRelational;
            return !bComparisonTail && cNext != '=';
        }

        std::optional<std::string> FindBracketProblem(std::string_view svCode)
        {
            std::vector<char> vOpen;
            for (size_t uPosition = 0; uPosition < svCode.size(); ++uPosition)
            {
                const char cCharacter = svCode[uPosition];
                if (cCharacter == '(' || cCharacter == '[')
                {
                    vOpen.push_back(cCharacter);
                    continue;
                }
                if (cCharacter == ')' || cCharacter == ']')
                {
                    const char cExpected = cCharacter == ')' ? '(' : '[';
                    if (vOpen.empty() || vOpen.back() != cExpected)
                    {
                        return std::format("CustomCode expression has an unbalanced '{}' at position {}", cCharacter, uPosition);
                    }
                    vOpen.pop_back();
                    continue;
                }
                if (cCharacter == ',' && vOpen.empty())
                {
                    return std::format("CustomCode expression must be a single expression; ',' at position {} is outside any parentheses", uPosition);
                }
            }
            if (!vOpen.empty())
            {
                return std::format("CustomCode expression has an unclosed '{}'", vOpen.back());
            }
            return std::nullopt;
        }
    }

    std::optional<std::string> MaterialCustomCodeRules::FindProblem(std::string_view svCode, uint8_t uInputCount)
    {
        if (svCode.empty())
        {
            return std::string("CustomCode expression must not be empty");
        }
        for (const std::string_view svForbidden : kForbiddenTokens)
        {
            if (svCode.find(svForbidden) != std::string_view::npos)
            {
                return std::format("CustomCode expression must be a single expression without '{}'", DescribeToken(svForbidden));
            }
        }
        for (size_t uPosition = svCode.find('='); uPosition != std::string_view::npos; uPosition = svCode.find('=', uPosition + 1))
        {
            if (IsAssignment(svCode, uPosition))
            {
                return std::format("CustomCode expression must not assign ('=' at position {})", uPosition);
            }
        }
        if (UsesIdentifier(svCode, kOutputIdentifier))
        {
            return std::format("CustomCode expression must not access '{}'", kOutputIdentifier);
        }

        for (size_t uPosition = svCode.find('$'); uPosition != std::string_view::npos; uPosition = svCode.find('$', uPosition + 1))
        {
            const char cDigit = uPosition + 1 < svCode.size() ? svCode[uPosition + 1] : '\0';
            if (!MaterialText::IsAsciiDigit(cDigit) || static_cast<uint8_t>(cDigit - '0') >= uInputCount)
            {
                return std::format("CustomCode placeholder at position {} must be $0..${} for its {} input(s)", uPosition, uInputCount == 0 ? 0 : uInputCount - 1, uInputCount);
            }
        }
        return FindBracketProblem(svCode);
    }

    bool MaterialCustomCodeRules::UsesIdentifier(std::string_view svCode, std::string_view svIdentifier) noexcept
    {
        size_t uPosition = 0;
        while (uPosition < svCode.size())
        {
            if (!MaterialText::IsIdentifierCharacter(svCode[uPosition]))
            {
                ++uPosition;
                continue;
            }
            const size_t uStart = uPosition;
            while (uPosition < svCode.size() && MaterialText::IsIdentifierCharacter(svCode[uPosition]))
            {
                ++uPosition;
            }
            const bool bMemberAccess = uStart > 0 && svCode[uStart - 1] == '.';
            if (!bMemberAccess && svCode.substr(uStart, uPosition - uStart) == svIdentifier)
            {
                return true;
            }
        }
        return false;
    }
}
