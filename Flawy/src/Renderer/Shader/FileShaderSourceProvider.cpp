#include "Flawy/Renderer/Shader/FileShaderSourceProvider.hpp"

#include "Flawy/Core/Logger.hpp"
#include "Renderer/Shader/ShaderSourcePath.hpp"

#include <exception>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <system_error>
#include <utility>

namespace Flawy::Renderer
{
    namespace
    {
        std::filesystem::path MakeAbsoluteRoot(const std::filesystem::path& oRoot)
        {
            std::error_code oError;
            const std::filesystem::path oAbsolute = std::filesystem::absolute(oRoot, oError);
            return oError ? oRoot : oAbsolute.lexically_normal();
        }

        std::string ToUtf8Path(const std::filesystem::path& oPath)
        {
            const std::u8string sPath = oPath.generic_u8string();
            return std::string(reinterpret_cast<const char*>(sPath.data()), sPath.size());
        }

        std::filesystem::path ToRelativePath(std::string_view svPath)
        {
            return std::filesystem::path(std::u8string(reinterpret_cast<const char8_t*>(svPath.data()), svPath.size()));
        }

        std::optional<std::string> ReadFile(const std::filesystem::path& oPath)
        {
            std::ifstream oStream(oPath, std::ios::binary);
            if (!oStream)
            {
                return std::nullopt;
            }

            std::string sCode((std::istreambuf_iterator<char>(oStream)), std::istreambuf_iterator<char>());
            if (oStream.bad())
            {
                return std::nullopt;
            }
            return sCode;
        }

        void LogUnreadable(const std::filesystem::path& oPath)
        {
            if (Core::Logger::IsInitialized())
            {
                Core::Logger::Get().warn("Shader source file {} exists but could not be read", ToUtf8Path(oPath));
            }
        }
    }

    FileShaderSourceProvider::FileShaderSourceProvider(std::vector<std::filesystem::path> vSearchRoots)
        : m_vSearchRoots(std::move(vSearchRoots))
    {
        for (std::filesystem::path& oRoot : m_vSearchRoots)
        {
            oRoot = MakeAbsoluteRoot(oRoot);
        }
    }

    std::optional<ShaderSource> FileShaderSourceProvider::Load(std::string_view svPath) const
    {
        try
        {
            const std::string sPath = ShaderSourcePath::Normalize(svPath);
            if (!ShaderSourcePath::IsContainedRelativePath(sPath))
            {
                return std::nullopt;
            }

            const std::filesystem::path oRelative = ToRelativePath(sPath);
            if (oRelative.is_absolute() || oRelative.has_root_name())
            {
                return std::nullopt;
            }

            for (const std::filesystem::path& oRoot : m_vSearchRoots)
            {
                const std::filesystem::path oCandidate = oRoot / oRelative;
                std::error_code oError;
                if (!std::filesystem::is_regular_file(oCandidate, oError))
                {
                    continue;
                }

                std::optional<std::string> optCode = ReadFile(oCandidate);
                if (!optCode)
                {
                    LogUnreadable(oCandidate);
                    return std::nullopt;
                }

                ShaderSource oSource;
                oSource.sName = std::string(ShaderSourcePath::GetModuleName(sPath));
                oSource.sPath = ToUtf8Path(oCandidate);
                oSource.sCode = std::move(*optCode);
                return oSource;
            }
            return std::nullopt;
        }
        catch (const std::exception&)
        {
            return std::nullopt;
        }
    }

    std::span<const std::filesystem::path> FileShaderSourceProvider::GetSearchRoots() const noexcept
    {
        return m_vSearchRoots;
    }
}
