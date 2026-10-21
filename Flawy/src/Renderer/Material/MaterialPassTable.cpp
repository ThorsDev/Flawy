#include "Renderer/Material/MaterialPassTable.hpp"

#include "Flawy/Renderer/Shader/EmbeddedShaderSourceProvider.hpp"

#include <format>
#include <optional>
#include <stdexcept>
#include <string>

namespace Flawy::Renderer
{
    namespace
    {
        struct PassModuleEntry
        {
            std::string_view svName;
            std::string_view svPath;
        };

        PassModuleEntry GetEntry(MaterialPass ePass) noexcept
        {
            switch (ePass)
            {
            case MaterialPass::Forward:
                return {"Forward", "Flawy/Passes/Forward.slang"};
            case MaterialPass::DepthOnly:
                return {"DepthOnly", "Flawy/Passes/DepthOnly.slang"};
            }
            return {"Unknown", ""};
        }
    }

    std::string_view MaterialPassTable::GetName(MaterialPass ePass) noexcept
    {
        return GetEntry(ePass).svName;
    }

    ShaderModuleSource MaterialPassTable::LoadModule(MaterialPass ePass)
    {
        const PassModuleEntry oEntry = GetEntry(ePass);
        const std::optional<ShaderSource> optSource = EmbeddedShaderSourceProvider().Load(oEntry.svPath);
        if (!optSource)
        {
            throw std::runtime_error(std::format("Embedded pass shader '{}' for material pass {} was not found", oEntry.svPath, oEntry.svName));
        }
        return {std::string(oEntry.svName), optSource->sPath, optSource->sCode};
    }
}
