#include "Renderer/Shader/SpirvShaderBackendEmitter.hpp"

#include "Flawy/Renderer/Shader/ShaderCompileError.hpp"

#include <cstring>
#include <format>
#include <unordered_map>

namespace Flawy::Renderer
{
    namespace
    {
        constexpr uint32_t kVertexUniformBinding = 0;
        constexpr uint32_t kFragmentUniformBinding = 1;
        constexpr uint32_t kTextureBindingBase = 2;
        constexpr uint32_t kSamplerBindingOffset = 16;
        constexpr uint32_t kDescriptorSet = 0;

        uint32_t GetUniformBinding(ShaderStage eStage) noexcept
        {
            return eStage == ShaderStage::Fragment ? kFragmentUniformBinding : kVertexUniformBinding;
        }

        class SpirvUniformBindingValidator final
        {
        public:
            explicit SpirvUniformBindingValidator(std::span<const std::byte> spanCode)
            {
                if (spanCode.size() % sizeof(uint32_t) != 0 || spanCode.size() < kHeaderWordCount * sizeof(uint32_t))
                {
                    throw ShaderCompileError("Generated SPIR-V has an invalid size");
                }

                m_vWords.resize(spanCode.size() / sizeof(uint32_t));
                std::memcpy(m_vWords.data(), spanCode.data(), spanCode.size());
                if (m_vWords[0] != kMagicNumber)
                {
                    throw ShaderCompileError("Generated SPIR-V has an invalid magic number");
                }
            }

            void Validate(uint32_t uExpectedBinding) const
            {
                std::unordered_map<uint32_t, uint32_t> mapBindings;
                std::unordered_map<uint32_t, uint32_t> mapDescriptorSets;
                std::vector<uint32_t> vUniformVariables;

                size_t uOffset = kHeaderWordCount;
                while (uOffset < m_vWords.size())
                {
                    const uint32_t uWordCount = m_vWords[uOffset] >> 16;
                    const uint32_t uOpcode = m_vWords[uOffset] & 0xffffu;
                    if (uWordCount == 0 || uOffset + uWordCount > m_vWords.size())
                    {
                        throw ShaderCompileError("Generated SPIR-V contains a malformed instruction");
                    }

                    if (uOpcode == kOpDecorate && uWordCount >= 4)
                    {
                        const uint32_t uTarget = m_vWords[uOffset + 1];
                        const uint32_t uDecoration = m_vWords[uOffset + 2];
                        if (uDecoration == kDecorationBinding)
                        {
                            mapBindings[uTarget] = m_vWords[uOffset + 3];
                        }
                        else if (uDecoration == kDecorationDescriptorSet)
                        {
                            mapDescriptorSets[uTarget] = m_vWords[uOffset + 3];
                        }
                    }
                    else if (uOpcode == kOpVariable && uWordCount >= 4 && m_vWords[uOffset + 3] == kStorageClassUniform)
                    {
                        vUniformVariables.push_back(m_vWords[uOffset + 2]);
                    }

                    uOffset += uWordCount;
                }

                for (const uint32_t uVariable : vUniformVariables)
                {
                    const uint32_t uBinding = mapBindings.contains(uVariable) ? mapBindings.at(uVariable) : 0;
                    const uint32_t uSet = mapDescriptorSets.contains(uVariable) ? mapDescriptorSets.at(uVariable) : 0;
                    if (uBinding != uExpectedBinding || uSet != kDescriptorSet)
                    {
                        throw ShaderCompileError(std::format("Global uniform block is bound to set {} binding {} but bgfx expects set {} binding {}",
                                                             uSet,
                                                             uBinding,
                                                             kDescriptorSet,
                                                             uExpectedBinding));
                    }
                }
            }

        private:
            static constexpr size_t kHeaderWordCount = 5;
            static constexpr uint32_t kMagicNumber = 0x07230203;
            static constexpr uint32_t kOpVariable = 59;
            static constexpr uint32_t kOpDecorate = 71;
            static constexpr uint32_t kDecorationBinding = 33;
            static constexpr uint32_t kDecorationDescriptorSet = 34;
            static constexpr uint32_t kStorageClassUniform = 2;

            std::vector<uint32_t> m_vWords;
        };
    }

    SlangCompileTarget SpirvShaderBackendEmitter::GetTargetFormat() const noexcept
    {
        return SLANG_SPIRV;
    }

    const char* SpirvShaderBackendEmitter::GetProfileName() const noexcept
    {
        return "spirv_1_3";
    }

    void SpirvShaderBackendEmitter::AppendTargetOptions(ShaderStage eStage, std::vector<slang::CompilerOptionEntry>& vOptions) const
    {
        slang::CompilerOptionEntry oBindGlobals;
        oBindGlobals.name = slang::CompilerOptionName::VulkanBindGlobals;
        oBindGlobals.value.kind = slang::CompilerOptionValueKind::Int;
        oBindGlobals.value.intValue0 = static_cast<int32_t>(GetUniformBinding(eStage));
        oBindGlobals.value.intValue1 = static_cast<int32_t>(kDescriptorSet);
        vOptions.push_back(oBindGlobals);
    }

    BgfxBindingRules SpirvShaderBackendEmitter::GetBindingRules(ShaderStage eStage) const noexcept
    {
        return {GetUniformBinding(eStage), kTextureBindingBase, kSamplerBindingOffset, SLANG_PARAMETER_CATEGORY_DESCRIPTOR_TABLE_SLOT, false};
    }

    std::vector<std::byte> SpirvShaderBackendEmitter::EmitShaderCode(std::span<const std::byte> spanTargetCode, ShaderStage eStage) const
    {
        SpirvUniformBindingValidator(spanTargetCode).Validate(GetUniformBinding(eStage));
        return {spanTargetCode.begin(), spanTargetCode.end()};
    }
}
