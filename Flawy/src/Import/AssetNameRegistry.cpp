#include "Import/AssetNameRegistry.hpp"

#include <algorithm>
#include <array>
#include <cstdint>
#include <format>

namespace Flawy::Import
{
    namespace
    {
        constexpr std::string_view kForbiddenCharacters = "<>:\"/\\|?*#";
        constexpr std::array<std::string_view, 22> kReservedDeviceNames = {
            "con",  "prn",  "aux",  "nul",  "com1", "com2", "com3", "com4", "com5", "com6", "com7",
            "com8", "com9", "lpt1", "lpt2", "lpt3", "lpt4", "lpt5", "lpt6", "lpt7", "lpt8", "lpt9",
        };
        constexpr unsigned char kFirstPrintableCharacter = 0x20;
        constexpr unsigned char kUtf8ContinuationMask = 0xc0;
        constexpr unsigned char kUtf8ContinuationValue = 0x80;

        bool IsForbidden(char cCharacter) noexcept
        {
            return static_cast<unsigned char>(cCharacter) < kFirstPrintableCharacter || kForbiddenCharacters.find(cCharacter) != std::string_view::npos;
        }

        bool IsTrimmed(char cCharacter) noexcept
        {
            return cCharacter == ' ' || cCharacter == '.';
        }

        bool IsUtf8Continuation(char cCharacter) noexcept
        {
            return (static_cast<unsigned char>(cCharacter) & kUtf8ContinuationMask) == kUtf8ContinuationValue;
        }
    }

    std::string AssetNameRegistry::Reserve(std::string_view svName, std::string_view svFallback)
    {
        std::string sBase = Sanitize(svName);
        if (sBase.empty())
        {
            sBase = Sanitize(svFallback);
        }
        if (sBase.empty())
        {
            sBase = "Asset";
        }

        std::string sName = sBase;
        for (uint32_t uSuffix = 2; m_setLowerNames.contains(ToLower(sName)); ++uSuffix)
        {
            sName = std::format("{}_{}", sBase, uSuffix);
        }
        m_setLowerNames.insert(ToLower(sName));
        return sName;
    }

    std::string AssetNameRegistry::Sanitize(std::string_view svName)
    {
        std::string sName;
        sName.reserve(svName.size());
        for (const char cCharacter : svName)
        {
            sName.push_back(IsForbidden(cCharacter) ? '_' : cCharacter);
        }

        if (sName.size() > kMaxNameLength)
        {
            size_t uLength = kMaxNameLength;
            while (uLength > 0 && IsUtf8Continuation(sName[uLength]))
            {
                --uLength;
            }
            sName.resize(uLength);
        }

        const auto itFirst = std::ranges::find_if_not(sName, IsTrimmed);
        sName.erase(sName.begin(), itFirst);
        while (!sName.empty() && IsTrimmed(sName.back()))
        {
            sName.pop_back();
        }

        const std::string sLower = ToLower(sName);
        const std::string_view svStem = std::string_view(sLower).substr(0, sLower.find('.'));
        if (std::ranges::find(kReservedDeviceNames, svStem) != kReservedDeviceNames.end())
        {
            sName.push_back('_');
        }
        return sName;
    }

    std::string AssetNameRegistry::ToLower(std::string_view svName)
    {
        std::string sLower(svName);
        std::ranges::transform(sLower, sLower.begin(), [](char cCharacter) {
            return cCharacter >= 'A' && cCharacter <= 'Z' ? static_cast<char>(cCharacter - 'A' + 'a') : cCharacter;
        });
        return sLower;
    }
}
