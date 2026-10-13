#include "Flawy/Core/BuildInfo.hpp"

#include <Jolt/Jolt.h>
#include <SDL3/SDL.h>
#include <angelscript.h>
#include <bgfx/bgfx.h>
#include <entt/config/version.h>
#include <glm/detail/setup.hpp>
#include <imgui.h>
#include <miniaudio.h>
#include <slang-tag-version.h>
#include <spdlog/version.h>

#include <format>

namespace Flawy::Core
{
    namespace
    {
        std::string FormatSdlVersion()
        {
            const int iVersion = SDL_GetVersion();
            return std::format("{}.{}.{}", SDL_VERSIONNUM_MAJOR(iVersion), SDL_VERSIONNUM_MINOR(iVersion), SDL_VERSIONNUM_MICRO(iVersion));
        }
    }

    std::string_view BuildInfo::GetEngineVersion() noexcept
    {
        return FLAWY_VERSION;
    }

    std::vector<DependencyVersion> BuildInfo::GetDependencyVersions()
    {
        return {
            {"SDL", FormatSdlVersion()},
            {"bgfx API", std::to_string(BGFX_API_VERSION)},
            {"miniaudio", ma_version_string()},
            {"EnTT", ENTT_VERSION},
            {"AngelScript", asGetLibraryVersion()},
            {"Jolt Physics", std::format("{}.{}.{}", JPH_VERSION_MAJOR, JPH_VERSION_MINOR, JPH_VERSION_PATCH)},
            {"GLM", std::format("{}.{}.{}", GLM_VERSION_MAJOR, GLM_VERSION_MINOR, GLM_VERSION_PATCH)},
            {"Dear ImGui", IMGUI_VERSION},
            {"spdlog", std::format("{}.{}.{}", SPDLOG_VER_MAJOR, SPDLOG_VER_MINOR, SPDLOG_VER_PATCH)},
            {"Slang", SLANG_TAG_VERSION},
        };
    }
}
