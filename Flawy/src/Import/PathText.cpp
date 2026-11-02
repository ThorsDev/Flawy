#include "Import/PathText.hpp"

namespace Flawy::Import
{
    std::string PathText::ToUtf8(const std::filesystem::path& oPath)
    {
        const std::u8string sUtf8 = oPath.u8string();
        return std::string(sUtf8.begin(), sUtf8.end());
    }

    std::string PathText::GetStem(const std::filesystem::path& oPath)
    {
        return ToUtf8(oPath.stem());
    }
}
