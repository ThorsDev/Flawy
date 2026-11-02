#include "Renderer/Shader/ShaderSourcePath.hpp"

#include <algorithm>

namespace Flawy::Renderer
{
    namespace
    {
        constexpr std::string_view kCurrentDirectoryPrefix = "./";
        constexpr std::string_view kParentDirectory = "..";
    }

    std::string ShaderSourcePath::Normalize(std::string_view svPath)
    {
        std::string sPath(svPath);
        std::ranges::replace(sPath, '\\', '/');
        while (std::string_view(sPath).starts_with(kCurrentDirectoryPrefix))
        {
            sPath.erase(0, kCurrentDirectoryPrefix.size());
        }
        return sPath;
    }

    std::string_view ShaderSourcePath::GetModuleName(std::string_view svPath) noexcept
    {
        const size_t uSeparator = svPath.find_last_of('/');
        if (uSeparator != std::string_view::npos)
        {
            svPath.remove_prefix(uSeparator + 1);
        }

        const size_t uExtension = svPath.find_last_of('.');
        if (uExtension != std::string_view::npos)
        {
            svPath = svPath.substr(0, uExtension);
        }
        return svPath;
    }

    bool ShaderSourcePath::IsContainedRelativePath(std::string_view svPath) noexcept
    {
        if (svPath.empty() || svPath.front() == '/' || svPath.front() == '\\')
        {
            return false;
        }

        bool bFirstSegment = true;
        while (true)
        {
            const size_t uSeparator = svPath.find_first_of("/\\");
            const std::string_view svSegment = svPath.substr(0, uSeparator);
            if (svSegment == kParentDirectory || (bFirstSegment && svSegment.find(':') != std::string_view::npos))
            {
                return false;
            }
            if (uSeparator == std::string_view::npos)
            {
                return true;
            }
            svPath.remove_prefix(uSeparator + 1);
            bFirstSegment = false;
        }
    }
}
