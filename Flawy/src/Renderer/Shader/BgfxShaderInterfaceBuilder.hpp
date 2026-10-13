#pragma once

#include "Flawy/Renderer/Shader/ShaderInterface.hpp"
#include "Flawy/Renderer/Shader/ShaderStage.hpp"
#include "Renderer/Shader/BgfxShaderInterface.hpp"
#include "Renderer/Shader/IShaderBackendEmitter.hpp"

#include <slang.h>

namespace Flawy::Renderer
{
    class BgfxShaderInterfaceBuilder final
    {
    public:
        [[nodiscard]] BgfxShaderInterface Build(slang::ProgramLayout& oLayout,
                                                slang::EntryPointReflection& oEntryPoint,
                                                slang::IMetadata* pMetadata,
                                                ShaderStage eStage,
                                                const BgfxBindingRules& oRules) const;

        [[nodiscard]] static ShaderInterface ToPublicInterface(const BgfxShaderInterface& oInterface);

    private:
        void CollectUniforms(slang::ProgramLayout& oLayout, slang::IMetadata* pMetadata, const BgfxBindingRules& oRules, BgfxShaderInterface& oInterface) const;
        void CollectVertexAttributes(slang::EntryPointReflection& oEntryPoint, BgfxShaderInterface& oInterface) const;
    };
}
