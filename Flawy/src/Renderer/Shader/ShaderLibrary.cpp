#include "Flawy/Renderer/Shader/ShaderLibrary.hpp"

#include "Flawy/Core/Logger.hpp"
#include "Flawy/Renderer/Shader/CompiledShader.hpp"
#include "Flawy/Renderer/Shader/EmbeddedShaderSourceProvider.hpp"
#include "Flawy/Renderer/Shader/FileShaderSourceProvider.hpp"
#include "Flawy/Renderer/Shader/ShaderSource.hpp"
#include "Flawy/Renderer/Shader/ShaderStage.hpp"
#include "Flawy/Renderer/ShaderProgram.hpp"
#include "Renderer/Shader/CachingShaderCompiler.hpp"
#include "Renderer/Shader/ShaderInterfaceText.hpp"
#include "Renderer/Shader/ShaderSourcePath.hpp"

#include <chrono>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace Flawy::Renderer
{
    namespace
    {
        constexpr std::string_view kShaderExtension = ".slang";
        constexpr std::string_view kVertexEntryPoint = "VertexMain";
        constexpr std::string_view kFragmentEntryPoint = "FragmentMain";
        constexpr std::string_view kEmbeddedOrigin = "embedded";

        std::string ToDisplayPath(const std::filesystem::path& oPath)
        {
            const std::u8string sPath = oPath.generic_u8string();
            return std::string(reinterpret_cast<const char*>(sPath.data()), sPath.size());
        }

        std::string DescribeSearchRoots(std::span<const std::filesystem::path> spanRoots)
        {
            std::string sText;
            for (const std::filesystem::path& oRoot : spanRoots)
            {
                sText.append(sText.empty() ? "" : "; ");
                sText.append(ToDisplayPath(oRoot));
            }
            return sText;
        }

        void ValidateProgramName(std::string_view svName)
        {
            if (svName.empty())
            {
                throw std::invalid_argument("Shader program name must not be empty");
            }
            if (svName.ends_with(kShaderExtension))
            {
                throw std::invalid_argument(std::format("Shader program name '{}' must not include the '{}' extension", svName, kShaderExtension));
            }
            if (!ShaderSourcePath::IsContainedRelativePath(ShaderSourcePath::Normalize(svName)))
            {
                throw std::invalid_argument(std::format("Shader program name '{}' must be a relative path without '..' segments", svName));
            }
        }
    }

    struct ShaderLibrary::Impl
    {
        struct ProgramRecord
        {
            std::string sName;
            ShaderSource oSource;
            std::string sOrigin;
            std::unique_ptr<ShaderProgram> upProgram;
        };

        struct ResolvedSource
        {
            ShaderSource oSource;
            std::string sOrigin;
        };

        Impl(ShaderCompiler& oShaderCompiler, ShaderCache& oShaderCache, const ShaderLibrarySpecification& oSpecification)
            : oCompiler(oShaderCompiler)
            , oCache(oShaderCache)
            , oFileProvider(oSpecification.vSearchRoots)
        {
        }

        [[nodiscard]] ShaderProgram* Find(std::string_view svName) const noexcept
        {
            for (const std::unique_ptr<ProgramRecord>& upRecord : vRecords)
            {
                if (upRecord->sName == svName)
                {
                    return upRecord->upProgram.get();
                }
            }
            return nullptr;
        }

        [[nodiscard]] ResolvedSource Resolve(std::string_view svName) const
        {
            const std::string sPath = std::format("{}{}", svName, kShaderExtension);
            std::optional<ShaderSource> optSource = oFileProvider.Load(sPath);
            if (optSource)
            {
                std::string sOrigin = optSource->sPath;
                return {std::move(*optSource), std::move(sOrigin)};
            }

            optSource = oEmbeddedProvider.Load(sPath);
            if (optSource)
            {
                return {std::move(*optSource), std::string(kEmbeddedOrigin)};
            }

            throw std::invalid_argument(std::format("Shader '{}' was not found in search roots [{}] or the embedded shader table",
                                                    svName,
                                                    DescribeSearchRoots(oFileProvider.GetSearchRoots())));
        }

        [[nodiscard]] ShaderProgram& Compile(std::string_view svName)
        {
            const std::chrono::steady_clock::time_point tStart = std::chrono::steady_clock::now();
            ResolvedSource oResolved = Resolve(svName);

            CachingShaderCompiler oCachingCompiler(oCompiler, oCache);
            const CachedShaderResult oVertex = oCachingCompiler.Compile(oResolved.oSource, ShaderStage::Vertex, kVertexEntryPoint);
            const CachedShaderResult oFragment = oCachingCompiler.Compile(oResolved.oSource, ShaderStage::Fragment, kFragmentEntryPoint);

            auto upRecord = std::make_unique<ProgramRecord>();
            upRecord->sName = std::string(svName);
            upRecord->oSource = std::move(oResolved.oSource);
            upRecord->sOrigin = std::move(oResolved.sOrigin);
            upRecord->upProgram = std::make_unique<ShaderProgram>(oVertex.oShader, oFragment.oShader);
            vRecords.push_back(std::move(upRecord));

            const std::chrono::duration<double, std::milli> tElapsed = std::chrono::steady_clock::now() - tStart;
            const uint32_t uCachedStages = (oVertex.bFromCache ? 1u : 0u) + (oFragment.bFromCache ? 1u : 0u);
            LogCompiled(*vRecords.back(), tElapsed.count(), CachingShaderCompiler::DescribeOrigin(uCachedStages, 2), oVertex.oShader, oFragment.oShader);
            return *vRecords.back()->upProgram;
        }

        void LogSearchRoots() const
        {
            if (!Core::Logger::IsInitialized())
            {
                return;
            }

            const std::span<const std::filesystem::path> spanRoots = oFileProvider.GetSearchRoots();
            if (spanRoots.empty())
            {
                Core::Logger::Get().info("Shader library search roots: none (embedded shaders only)");
                return;
            }
            Core::Logger::Get().info("Shader library search roots: {}", DescribeSearchRoots(spanRoots));
        }

        static void LogCompiled(const ProgramRecord& oRecord,
                                double dMilliseconds,
                                std::string_view svCacheOrigin,
                                const CompiledShader& oVertexShader,
                                const CompiledShader& oFragmentShader)
        {
            if (!Core::Logger::IsInitialized())
            {
                return;
            }

            auto& oLogger = Core::Logger::Get();
            oLogger.info("Compiled shader program '{}' from {} in {:.1f} ms ({})", oRecord.sName, oRecord.sOrigin, dMilliseconds, svCacheOrigin);
            for (const CompiledShader* pShader : {&oVertexShader, &oFragmentShader})
            {
                oLogger.debug("  {}::{} ({} bytes): {}",
                              pShader->sName,
                              pShader->sEntryPoint,
                              pShader->vBgfxBinary.size(),
                              ShaderInterfaceText::DescribeUniforms(pShader->oInterface));
            }
        }

        ShaderCompiler& oCompiler;
        ShaderCache& oCache;
        EmbeddedShaderSourceProvider oEmbeddedProvider;
        FileShaderSourceProvider oFileProvider;
        std::vector<std::unique_ptr<ProgramRecord>> vRecords;
    };

    ShaderLibrary::ShaderLibrary(ShaderCompiler& oCompiler, ShaderCache& oCache, const ShaderLibrarySpecification& oSpecification)
        : m_upImpl(std::make_unique<Impl>(oCompiler, oCache, oSpecification))
    {
        m_upImpl->LogSearchRoots();
    }

    ShaderLibrary::~ShaderLibrary() = default;

    ShaderProgram& ShaderLibrary::GetProgram(std::string_view svName)
    {
        ValidateProgramName(svName);
        const std::string sName = ShaderSourcePath::Normalize(svName);
        ShaderProgram* pProgram = m_upImpl->Find(sName);
        return pProgram != nullptr ? *pProgram : m_upImpl->Compile(sName);
    }

    uint32_t ShaderLibrary::GetProgramCount() const noexcept
    {
        return static_cast<uint32_t>(m_upImpl->vRecords.size());
    }

    std::span<const std::filesystem::path> ShaderLibrary::GetSearchRoots() const noexcept
    {
        return m_upImpl->oFileProvider.GetSearchRoots();
    }
}
