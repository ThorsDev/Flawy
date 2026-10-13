#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace Flawy::Core
{
    struct DependencyVersion
    {
        std::string sName;
        std::string sVersion;
    };

    class BuildInfo final
    {
    public:
        BuildInfo() = delete;

        [[nodiscard]] static std::string_view GetEngineVersion() noexcept;
        [[nodiscard]] static std::vector<DependencyVersion> GetDependencyVersions();
    };
}
