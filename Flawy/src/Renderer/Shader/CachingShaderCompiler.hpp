#pragma once

#include "Flawy/Renderer/Shader/CompiledShader.hpp"
#include "Flawy/Renderer/Shader/ShaderSource.hpp"
#include "Flawy/Renderer/Shader/ShaderStage.hpp"
#include "Flawy/Renderer/Shader/SpecializedShaderRequest.hpp"

#include <cstdint>
#include <string_view>

namespace Flawy::Renderer
{
    class ShaderCache;
    class ShaderCompiler;

    struct CachedShaderResult
    {
        CompiledShader oShader;
        bool bFromCache = false;
    };

    class CachingShaderCompiler final
    {
    public:
        CachingShaderCompiler(ShaderCompiler& oCompiler, ShaderCache& oCache) noexcept;

        [[nodiscard]] CachedShaderResult Compile(const ShaderSource& oSource, ShaderStage eStage, std::string_view svEntryPoint);
        [[nodiscard]] CachedShaderResult CompileSpecialized(const SpecializedShaderRequest& oRequest);

        [[nodiscard]] static std::string_view DescribeOrigin(uint32_t uCachedStages, uint32_t uStageCount) noexcept;

    private:
        template <typename TKeyFactory, typename TCompile> [[nodiscard]] CachedShaderResult FindOrCompile(TKeyFactory&& fnMakeKey, TCompile&& fnCompile);

        ShaderCompiler& m_oCompiler;
        ShaderCache& m_oCache;
    };
}
