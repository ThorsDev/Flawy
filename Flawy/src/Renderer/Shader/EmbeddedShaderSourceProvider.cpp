#include "Flawy/Renderer/Shader/EmbeddedShaderSourceProvider.hpp"

#include "Renderer/Shader/EmbeddedShaderTable.hpp"

#include <string>

namespace Flawy::Renderer
{
    namespace
    {
        std::string_view GetModuleName(std::string_view svPath) noexcept
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
    }

    std::optional<ShaderSource> EmbeddedShaderSourceProvider::Load(std::string_view svPath) const
    {
        for (const EmbeddedShaderEntry& oEntry : EmbeddedShaderTable::GetEntries())
        {
            if (svPath != oEntry.szPath)
            {
                continue;
            }

            ShaderSource oSource;
            oSource.sName = std::string(GetModuleName(svPath));
            oSource.sPath = std::string(svPath);
            oSource.sCode.assign(reinterpret_cast<const char*>(oEntry.pData), oEntry.uSize);
            return oSource;
        }
        return std::nullopt;
    }

    std::vector<std::string_view> EmbeddedShaderSourceProvider::ListPaths() const
    {
        std::vector<std::string_view> vPaths;
        for (const EmbeddedShaderEntry& oEntry : EmbeddedShaderTable::GetEntries())
        {
            vPaths.emplace_back(oEntry.szPath);
        }
        return vPaths;
    }
}
