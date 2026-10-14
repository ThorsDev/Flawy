#include "Renderer/Shader/BgfxShaderInterfaceBuilder.hpp"

#include "Flawy/Renderer/Shader/ShaderCompileError.hpp"
#include "Renderer/Shader/BgfxAttributeTable.hpp"

#include <bgfx/bgfx.h>

#include <format>
#include <limits>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Flawy::Renderer
{
    namespace
    {
        constexpr size_t kUniformRegisterSize = 16;
        constexpr size_t kMaxUniformNameLength = 255;
        constexpr size_t kMaxUniformArraySize = 255;
        constexpr size_t kMaxVertexAttributeLocation = 31;
        constexpr std::string_view kAllowedUniformTypes = "float4, float3x3, float4x4 or fixed-size arrays of them";
        constexpr std::string_view kSystemValuePrefix = "SV_";
        constexpr std::string_view kSamplerSuffix = "Sampler";
        constexpr std::string_view kTextureDeclarationHint = "declare textures with FLAWY_SAMPLER2D(name, stage) from \"Flawy/Sampler.slang\"";
        constexpr uint32_t kMaxTextureStages = 16;
        constexpr uint8_t kTextureDimension2D = 0x02;

        struct UniformShape
        {
            bgfx::UniformType::Enum eType = bgfx::UniformType::Count;
            uint16_t uRegistersPerElement = 0;
        };

        std::string_view ToView(const char* szText) noexcept
        {
            return szText != nullptr ? std::string_view(szText) : std::string_view();
        }

        size_t AlignUp(size_t uValue, size_t uAlignment) noexcept
        {
            return (uValue + uAlignment - 1) / uAlignment * uAlignment;
        }

        bool HasCategory(slang::VariableLayoutReflection& oVariable, slang::ParameterCategory eCategory)
        {
            const unsigned int uCount = oVariable.getCategoryCount();
            for (unsigned int uIndex = 0; uIndex < uCount; ++uIndex)
            {
                if (oVariable.getCategoryByIndex(uIndex) == eCategory)
                {
                    return true;
                }
            }
            return false;
        }

        std::optional<UniformShape> DetermineShape(slang::TypeReflection& oType)
        {
            if (oType.getScalarType() != slang::TypeReflection::ScalarType::Float32)
            {
                return std::nullopt;
            }

            switch (oType.getKind())
            {
            case slang::TypeReflection::Kind::Vector:
                if (oType.getElementCount() == 4)
                {
                    return UniformShape{bgfx::UniformType::Vec4, 1};
                }
                break;
            case slang::TypeReflection::Kind::Matrix:
                if (oType.getRowCount() == 4 && oType.getColumnCount() == 4)
                {
                    return UniformShape{bgfx::UniformType::Mat4, 4};
                }
                if (oType.getRowCount() == 3 && oType.getColumnCount() == 3)
                {
                    return UniformShape{bgfx::UniformType::Mat3, 3};
                }
                break;
            default:
                break;
            }
            return std::nullopt;
        }

        BgfxUniform BuildUniform(slang::VariableLayoutReflection& oVariable)
        {
            const std::string sName(ToView(oVariable.getName()));
            if (sName.empty() || sName.size() > kMaxUniformNameLength)
            {
                throw ShaderCompileError(std::format("Uniform name '{}' must be between 1 and {} characters", sName, kMaxUniformNameLength));
            }

            slang::TypeReflection* pType = oVariable.getType();
            size_t uElementCount = 1;
            if (pType->getKind() == slang::TypeReflection::Kind::Array)
            {
                uElementCount = pType->getElementCount();
                pType = pType->getElementType();
                if (uElementCount == 0 || uElementCount > kMaxUniformArraySize)
                {
                    throw ShaderCompileError(std::format("Uniform '{}' must be a fixed-size array with 1 to {} elements", sName, kMaxUniformArraySize));
                }
            }

            const std::optional<UniformShape> optShape = DetermineShape(*pType);
            if (!optShape)
            {
                throw ShaderCompileError(std::format("Uniform '{}' has an unsupported type; bgfx uniforms must be {}", sName, kAllowedUniformTypes));
            }

            const size_t uOffset = oVariable.getOffset(slang::ParameterCategory::Uniform);
            if (uOffset > std::numeric_limits<uint16_t>::max())
            {
                throw ShaderCompileError(std::format("Uniform '{}' lies outside the 64 KiB uniform block", sName));
            }

            BgfxUniform oUniform;
            oUniform.sName = sName;
            oUniform.uType = static_cast<uint8_t>(optShape->eType);
            oUniform.uNum = static_cast<uint8_t>(uElementCount);
            oUniform.uRegIndex = static_cast<uint16_t>(uOffset);
            oUniform.uRegCount = static_cast<uint16_t>(optShape->uRegistersPerElement * uElementCount);
            return oUniform;
        }

        [[noreturn]] void RejectUnsupportedParameter(slang::VariableLayoutReflection& oVariable)
        {
            throw ShaderCompileError(std::format("Shader parameter '{}' is not a plain uniform; bgfx uniforms must be {}", ToView(oVariable.getName()), kAllowedUniformTypes));
        }

        bool IsUniformCandidate(slang::VariableLayoutReflection& oVariable)
        {
            switch (oVariable.getTypeLayout()->getKind())
            {
            case slang::TypeReflection::Kind::Scalar:
            case slang::TypeReflection::Kind::Vector:
            case slang::TypeReflection::Kind::Matrix:
            case slang::TypeReflection::Kind::Array:
                return HasCategory(oVariable, slang::ParameterCategory::Uniform);
            default:
                return false;
            }
        }

        bool IsLocationUsed(slang::IMetadata* pMetadata, SlangParameterCategory eCategory, SlangUInt uIndex)
        {
            if (pMetadata == nullptr)
            {
                return true;
            }

            bool bUsed = true;
            const SlangResult iResult = pMetadata->isParameterLocationUsed(eCategory, 0, uIndex, bUsed);
            return SLANG_FAILED(iResult) || bUsed;
        }

        size_t GetBinding(slang::VariableLayoutReflection& oVariable, SlangParameterCategory eCategory)
        {
            const size_t uSpace = oVariable.getBindingSpace(eCategory);
            if (uSpace != 0)
            {
                throw ShaderCompileError(std::format("Shader parameter '{}' is bound to register space / descriptor set {} but bgfx only uses space 0; {}",
                                                     ToView(oVariable.getName()),
                                                     uSpace,
                                                     kTextureDeclarationHint));
            }
            return oVariable.getOffset(static_cast<slang::ParameterCategory>(eCategory));
        }

        class TextureBindingCollector final
        {
        public:
            TextureBindingCollector(slang::IMetadata* pMetadata, const BgfxBindingRules& oRules, BgfxShaderInterface& oInterface) noexcept
                : m_pMetadata(pMetadata)
                , m_oRules(oRules)
                , m_oInterface(oInterface)
            {
            }

            void AddTexture(slang::VariableLayoutReflection& oTexture)
            {
                m_vTextures.push_back(&oTexture);
            }

            void AddSampler(slang::VariableLayoutReflection& oSampler)
            {
                m_vSamplers.push_back(&oSampler);
            }

            void Collect()
            {
                std::vector<bool> vSamplerPaired(m_vSamplers.size(), false);
                std::vector<std::string_view> vStageOwners(kMaxTextureStages);

                for (slang::VariableLayoutReflection* pTexture : m_vTextures)
                {
                    const std::string_view svName = ToView(pTexture->getName());
                    RequireTexture2D(*pTexture, svName);

                    const size_t uBinding = GetBinding(*pTexture, m_oRules.eTextureCategory);
                    const uint32_t uStage = ResolveStage(uBinding, svName);
                    if (!vStageOwners[uStage].empty())
                    {
                        throw ShaderCompileError(std::format("Textures '{}' and '{}' both use texture stage {}", vStageOwners[uStage], svName, uStage));
                    }
                    vStageOwners[uStage] = svName;

                    const size_t uSamplerIndex = FindPairedSampler(svName, uBinding);
                    vSamplerPaired[uSamplerIndex] = true;

                    if (IsLocationUsed(m_pMetadata, m_oRules.eTextureCategory, uBinding))
                    {
                        m_oInterface.vUniforms.push_back(BuildSamplerUniform(svName, uBinding, uStage));
                    }
                }

                for (size_t uIndex = 0; uIndex < m_vSamplers.size(); ++uIndex)
                {
                    if (!vSamplerPaired[uIndex])
                    {
                        throw ShaderCompileError(std::format("Sampler '{}' has no matching texture; {}", ToView(m_vSamplers[uIndex]->getName()), kTextureDeclarationHint));
                    }
                }
            }

        private:
            static void RequireTexture2D(slang::VariableLayoutReflection& oTexture, std::string_view svName)
            {
                slang::TypeLayoutReflection* pTypeLayout = oTexture.getTypeLayout();
                const bool bTexture2D = pTypeLayout->getResourceShape() == SLANG_TEXTURE_2D;
                const bool bReadOnly = pTypeLayout->getResourceAccess() == SLANG_RESOURCE_ACCESS_READ;
                if (!bTexture2D || !bReadOnly)
                {
                    throw ShaderCompileError(std::format("Shader parameter '{}' is an unsupported resource; only Texture2D is supported in this phase", svName));
                }
            }

            [[nodiscard]] uint32_t ResolveStage(size_t uBinding, std::string_view svName) const
            {
                if (uBinding < m_oRules.uTextureBindingBase || uBinding - m_oRules.uTextureBindingBase >= kMaxTextureStages)
                {
                    throw ShaderCompileError(std::format("Texture '{}' is bound to slot {} which is outside the bgfx texture stages 0-{}; {}",
                                                         svName,
                                                         uBinding,
                                                         kMaxTextureStages - 1,
                                                         kTextureDeclarationHint));
                }
                return static_cast<uint32_t>(uBinding - m_oRules.uTextureBindingBase);
            }

            [[nodiscard]] size_t FindPairedSampler(std::string_view svTextureName, size_t uTextureBinding) const
            {
                const std::string sSamplerName = std::string(svTextureName) + std::string(kSamplerSuffix);
                const size_t uExpectedBinding = uTextureBinding + m_oRules.uSamplerBindingOffset;
                for (size_t uIndex = 0; uIndex < m_vSamplers.size(); ++uIndex)
                {
                    slang::VariableLayoutReflection& oSampler = *m_vSamplers[uIndex];
                    if (ToView(oSampler.getName()) != sSamplerName)
                    {
                        continue;
                    }

                    const size_t uSamplerBinding = GetBinding(oSampler, m_oRules.eSamplerCategory);
                    if (uSamplerBinding != uExpectedBinding)
                    {
                        throw ShaderCompileError(std::format("Sampler '{}' is bound to slot {} but texture '{}' requires it at slot {}; {}",
                                                             sSamplerName,
                                                             uSamplerBinding,
                                                             svTextureName,
                                                             uExpectedBinding,
                                                             kTextureDeclarationHint));
                    }
                    return uIndex;
                }
                throw ShaderCompileError(std::format("Texture '{}' has no sampler named '{}'; {}", svTextureName, sSamplerName, kTextureDeclarationHint));
            }

            [[nodiscard]] static BgfxUniform BuildSamplerUniform(std::string_view svName, size_t uBinding, uint32_t uStage)
            {
                if (svName.empty() || svName.size() > kMaxUniformNameLength)
                {
                    throw ShaderCompileError(std::format("Texture name '{}' must be between 1 and {} characters", svName, kMaxUniformNameLength));
                }

                BgfxUniform oUniform;
                oUniform.sName = std::string(svName);
                oUniform.uType = static_cast<uint8_t>(bgfx::UniformType::Sampler);
                oUniform.uNum = 1;
                oUniform.uRegIndex = static_cast<uint16_t>(uBinding);
                oUniform.uRegCount = 1;
                oUniform.uTexComponent = 0;
                oUniform.uTexDimension = kTextureDimension2D;
                oUniform.uTexFormat = 0;
                oUniform.uStage = static_cast<uint8_t>(uStage);
                return oUniform;
            }

            slang::IMetadata* m_pMetadata;
            const BgfxBindingRules& m_oRules;
            BgfxShaderInterface& m_oInterface;
            std::vector<slang::VariableLayoutReflection*> m_vTextures;
            std::vector<slang::VariableLayoutReflection*> m_vSamplers;
        };

        class VertexAttributeCollector final
        {
        public:
            explicit VertexAttributeCollector(BgfxShaderInterface& oInterface) noexcept
                : m_oInterface(oInterface)
            {
            }

            void AddParameter(slang::VariableLayoutReflection& oParameter)
            {
                slang::TypeLayoutReflection* pTypeLayout = oParameter.getTypeLayout();
                const size_t uBaseLocation = oParameter.getOffset(slang::ParameterCategory::VaryingInput);
                if (pTypeLayout->getKind() != slang::TypeReflection::Kind::Struct)
                {
                    AddVarying(oParameter, uBaseLocation);
                    return;
                }

                const unsigned int uFieldCount = pTypeLayout->getFieldCount();
                for (unsigned int uField = 0; uField < uFieldCount; ++uField)
                {
                    slang::VariableLayoutReflection* pField = pTypeLayout->getFieldByIndex(uField);
                    AddVarying(*pField, uBaseLocation + pField->getOffset(slang::ParameterCategory::VaryingInput));
                }
            }

        private:
            void AddVarying(slang::VariableLayoutReflection& oVariable, size_t uLocation)
            {
                if (!HasCategory(oVariable, slang::ParameterCategory::VaryingInput))
                {
                    return;
                }

                const std::string_view svName = ToView(oVariable.getName());
                const std::string_view svSemantic = ToView(oVariable.getSemanticName());
                if (svSemantic.empty())
                {
                    throw ShaderCompileError(std::format("Vertex input '{}' has no semantic", svName));
                }
                if (svSemantic.starts_with(kSystemValuePrefix))
                {
                    return;
                }

                const std::optional<VertexAttribute> optAttribute = BgfxAttributeTable::FromSemantic(svSemantic, oVariable.getSemanticIndex());
                if (!optAttribute)
                {
                    throw ShaderCompileError(std::format("Vertex input '{}' uses semantic '{}{}' which has no bgfx attribute", svName, svSemantic, oVariable.getSemanticIndex()));
                }
                if (uLocation > kMaxVertexAttributeLocation)
                {
                    throw ShaderCompileError(std::format("Vertex input '{}' has location {} which exceeds the bgfx attribute limit", svName, uLocation));
                }

                if (m_oInterface.vAttributeIds.size() <= uLocation)
                {
                    m_oInterface.vAttributeIds.resize(uLocation + 1, BgfxAttributeTable::kUnknownAttributeId);
                }
                m_oInterface.vAttributeIds[uLocation] = BgfxAttributeTable::ToAttributeId(*optAttribute);
            }

            BgfxShaderInterface& m_oInterface;
        };

        ShaderUniformType ToPublicUniformType(uint8_t uType) noexcept
        {
            switch (static_cast<bgfx::UniformType::Enum>(uType))
            {
            case bgfx::UniformType::Mat3:
                return ShaderUniformType::Mat3;
            case bgfx::UniformType::Mat4:
                return ShaderUniformType::Mat4;
            case bgfx::UniformType::Sampler:
                return ShaderUniformType::Sampler;
            default:
                return ShaderUniformType::Vec4;
            }
        }
    }

    BgfxShaderInterface BgfxShaderInterfaceBuilder::Build(slang::ProgramLayout& oLayout,
                                                          slang::EntryPointReflection& oEntryPoint,
                                                          slang::IMetadata* pMetadata,
                                                          ShaderStage eStage,
                                                          const BgfxBindingRules& oRules) const
    {
        BgfxShaderInterface oInterface;
        CollectUniforms(oLayout, pMetadata, oRules, oInterface);
        if (eStage == ShaderStage::Vertex)
        {
            CollectVertexAttributes(oEntryPoint, oInterface);
        }
        return oInterface;
    }

    ShaderInterface BgfxShaderInterfaceBuilder::ToPublicInterface(const BgfxShaderInterface& oInterface)
    {
        ShaderInterface oPublic;
        oPublic.uUniformBufferSize = oInterface.uUniformBufferSize;
        for (const BgfxUniform& oUniform : oInterface.vUniforms)
        {
            oPublic.vUniforms.push_back({oUniform.sName, ToPublicUniformType(oUniform.uType), oUniform.uNum, oUniform.uStage, BgfxAttributeTable::IsPredefinedUniform(oUniform.sName)});
        }
        for (const uint16_t uAttributeId : oInterface.vAttributeIds)
        {
            const std::optional<VertexAttribute> optAttribute = BgfxAttributeTable::FromAttributeId(uAttributeId);
            if (optAttribute)
            {
                oPublic.vVertexAttributes.push_back(*optAttribute);
            }
        }
        return oPublic;
    }

    void BgfxShaderInterfaceBuilder::CollectUniforms(slang::ProgramLayout& oLayout,
                                                     slang::IMetadata* pMetadata,
                                                     const BgfxBindingRules& oRules,
                                                     BgfxShaderInterface& oInterface) const
    {
        TextureBindingCollector oTextures(pMetadata, oRules, oInterface);
        const unsigned int uParameterCount = oLayout.getParameterCount();
        for (unsigned int uIndex = 0; uIndex < uParameterCount; ++uIndex)
        {
            slang::VariableLayoutReflection& oParameter = *oLayout.getParameterByIndex(uIndex);
            switch (oParameter.getTypeLayout()->getKind())
            {
            case slang::TypeReflection::Kind::Resource:
                oTextures.AddTexture(oParameter);
                continue;
            case slang::TypeReflection::Kind::SamplerState:
                oTextures.AddSampler(oParameter);
                continue;
            default:
                break;
            }

            if (!IsUniformCandidate(oParameter))
            {
                RejectUnsupportedParameter(oParameter);
            }
            oInterface.vUniforms.push_back(BuildUniform(oParameter));
        }

        if (!oInterface.vUniforms.empty() && !IsLocationUsed(pMetadata, oRules.eUniformBlockCategory, oRules.uUniformBlockBinding))
        {
            oInterface.vUniforms.clear();
        }
        const bool bHasPlainUniforms = !oInterface.vUniforms.empty();
        oTextures.Collect();

        if (!bHasPlainUniforms)
        {
            return;
        }

        const SlangUInt uBinding = oLayout.getGlobalConstantBufferBinding();
        if (oRules.bReflectionReportsUniformBlockBinding && uBinding != oRules.uUniformBlockBinding)
        {
            throw ShaderCompileError(std::format("Global uniform block is bound to slot {} but bgfx expects slot {}", uBinding, oRules.uUniformBlockBinding));
        }

        const size_t uBufferSize = AlignUp(oLayout.getGlobalConstantBufferSize(), kUniformRegisterSize);
        if (uBufferSize > std::numeric_limits<uint16_t>::max())
        {
            throw ShaderCompileError(std::format("Global uniform block is {} bytes, which exceeds the bgfx limit", uBufferSize));
        }
        oInterface.uUniformBufferSize = static_cast<uint16_t>(uBufferSize);
    }

    void BgfxShaderInterfaceBuilder::CollectVertexAttributes(slang::EntryPointReflection& oEntryPoint, BgfxShaderInterface& oInterface) const
    {
        VertexAttributeCollector oCollector(oInterface);
        const unsigned int uParameterCount = oEntryPoint.getParameterCount();
        for (unsigned int uIndex = 0; uIndex < uParameterCount; ++uIndex)
        {
            oCollector.AddParameter(*oEntryPoint.getParameterByIndex(uIndex));
        }
    }
}
