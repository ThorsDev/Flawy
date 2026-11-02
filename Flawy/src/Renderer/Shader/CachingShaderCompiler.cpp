#include "Renderer/Shader/CachingShaderCompiler.hpp"

#include "Flawy/Renderer/Shader/ShaderCache.hpp"
#include "Flawy/Renderer/Shader/ShaderCompiler.hpp"

#include <optional>
#include <utility>

namespace Flawy::Renderer
{
    CachingShaderCompiler::CachingShaderCompiler(ShaderCompiler& oCompiler, ShaderCache& oCache) noexcept
        : m_oCompiler(oCompiler)
        , m_oCache(oCache)
    {
    }

    template <typename TKeyFactory, typename TCompile> CachedShaderResult CachingShaderCompiler::FindOrCompile(TKeyFactory&& fnMakeKey, TCompile&& fnCompile)
    {
        if (!m_oCache.IsEnabled())
        {
            return {fnCompile(), false};
        }

        const ShaderCacheKey oCacheKey = fnMakeKey();
        std::optional<CompiledShader> optCached = m_oCache.Find(oCacheKey);
        if (optCached)
        {
            return {std::move(*optCached), true};
        }

        CachedShaderResult oResult{fnCompile(), false};
        m_oCache.Store(oCacheKey, oResult.oShader);
        return oResult;
    }

    CachedShaderResult CachingShaderCompiler::Compile(const ShaderSource& oSource, ShaderStage eStage, std::string_view svEntryPoint)
    {
        return FindOrCompile([&]() { return ShaderCacheKey::FromSource(oSource, eStage, svEntryPoint, m_oCompiler.GetCacheFingerprint()); },
                             [&]() { return m_oCompiler.Compile(oSource, eStage, svEntryPoint); });
    }

    CachedShaderResult CachingShaderCompiler::CompileSpecialized(const SpecializedShaderRequest& oRequest)
    {
        return FindOrCompile([&]() { return ShaderCacheKey::FromRequest(oRequest, m_oCompiler.GetCacheFingerprint()); },
                             [&]() { return m_oCompiler.CompileSpecialized(oRequest); });
    }

    std::string_view CachingShaderCompiler::DescribeOrigin(uint32_t uCachedStages, uint32_t uStageCount) noexcept
    {
        if (uCachedStages == uStageCount)
        {
            return "cache";
        }
        return uCachedStages == 0 ? "compiled" : "partially cached";
    }
}
