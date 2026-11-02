#include "Flawy/Renderer/Shader/EmbeddedShaderSourceProvider.hpp"

#include "Renderer/Shader/EmbeddedShaderTable.hpp"
#include "Renderer/Shader/ShaderSourcePath.hpp"

#include <string>

namespace Flawy::Renderer
{
    std::optional<ShaderSource> EmbeddedShaderSourceProvider::Load(std::string_view svPath) const
    {
        for (const EmbeddedShaderEntry& oEntry : EmbeddedShaderTable::GetEntries())
        {
            if (svPath != oEntry.szPath)
            {
                continue;
            }

            ShaderSource oSource;
            oSource.sName = std::string(ShaderSourcePath::GetModuleName(svPath));
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
