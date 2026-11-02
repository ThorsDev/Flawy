#include "Flawy/Renderer/Material/Material.hpp"

#include "Flawy/Core/Logger.hpp"
#include "Flawy/Renderer/Material/MaterialCompiler.hpp"
#include "Flawy/Renderer/Material/MaterialSystem.hpp"
#include "Flawy/Renderer/Shader/CompiledShader.hpp"
#include "Flawy/Renderer/Shader/SpecializedShaderRequest.hpp"
#include "Flawy/Renderer/ShaderProgram.hpp"
#include "Renderer/Material/MaterialPassTable.hpp"
#include "Renderer/Material/MaterialShaderNames.hpp"
#include "Renderer/Material/MaterialText.hpp"
#include "Renderer/Material/VertexFactoryTable.hpp"
#include "Renderer/Shader/CachingShaderCompiler.hpp"
#include "Renderer/Shader/ShaderInterfaceText.hpp"

#include <algorithm>
#include <chrono>
#include <format>
#include <iterator>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Flawy::Renderer
{
    namespace
    {
        constexpr std::string_view kVertexEntryPoint = "VertexMain";
        constexpr std::string_view kFragmentEntryPoint = "FragmentMain";
        constexpr std::string_view kMaterialModuleDirectory = "Materials/";

        void ValidateDescriptor(const MaterialDescriptor& oDescriptor)
        {
            if (!MaterialText::IsIdentifier(oDescriptor.sModuleName))
            {
                throw std::invalid_argument(std::format("Material module name '{}' must be a valid Slang identifier", oDescriptor.sModuleName));
            }
            if (oDescriptor.sSlangSource.find(std::format("struct {}", oDescriptor.sModuleName)) == std::string::npos)
            {
                throw std::invalid_argument(std::format("Material module '{}' must declare 'public struct {} : IMaterial'", oDescriptor.sModuleName, oDescriptor.sModuleName));
            }
            oDescriptor.oParameters.Validate();
        }

        std::string DescribeKey(const MaterialPermutationKey& oKey)
        {
            return std::format("[{}, {}, set{}]", MaterialPassTable::GetName(oKey.ePass), VertexFactoryTable::GetName(oKey.eVertexFactory), oKey.uStaticSwitchSet);
        }

        std::string DescribeSwitches(std::span<const MaterialStaticSwitchValue> spanValues)
        {
            std::string sText;
            for (const MaterialStaticSwitchValue& oValue : spanValues)
            {
                sText.append(sText.empty() ? "" : ", ");
                sText.append(std::format("{}={}", oValue.sName, oValue.bValue));
            }
            return sText.empty() ? std::string("none") : sText;
        }

        class MaterialInterfaceVerifier final
        {
        public:
            MaterialInterfaceVerifier(std::string_view svMaterialName, const ShaderProgram& oProgram) noexcept
                : m_svMaterialName(svMaterialName)
                , m_oProgram(oProgram)
            {
            }

            void Verify(const MaterialParameterLayout& oLayout) const
            {
                if (oLayout.uVec4SlotCount > 0)
                {
                    VerifyValueSlots(oLayout.uVec4SlotCount);
                }
                for (const MaterialTextureParameter& oTexture : oLayout.vTextures)
                {
                    VerifyTexture(oTexture);
                }
                VerifyNoUndeclaredTextures(oLayout);
                VerifyEngineUniform(MaterialShaderNames::kViewParamsUniform, MaterialShaderNames::kViewParamsCount);
                VerifyEngineUniform(MaterialShaderNames::kLightParamsUniform, MaterialShaderNames::kLightParamsCount);
            }

        private:
            void VerifyValueSlots(uint8_t uSlotCount) const
            {
                const std::optional<uint32_t> optIndex = m_oProgram.FindUniform(MaterialShaderNames::kMaterialParamsUniform);
                if (!optIndex)
                {
                    return;
                }

                const ShaderUniform& oUniform = m_oProgram.GetUniforms()[*optIndex];
                if (oUniform.eType != ShaderUniformType::Vec4 || oUniform.uCount < uSlotCount)
                {
                    throw std::logic_error(std::format("Material '{}' declares {} vec4 parameter slot(s) but its shader declares {} as {}[{}]",
                                                       m_svMaterialName,
                                                       uSlotCount,
                                                       MaterialShaderNames::kMaterialParamsUniform,
                                                       ShaderInterfaceText::GetTypeName(oUniform.eType),
                                                       oUniform.uCount));
                }
            }

            void VerifyTexture(const MaterialTextureParameter& oTexture) const
            {
                const std::string sSampler = MaterialShaderNames::GetTextureSamplerName(oTexture.uStage);
                const std::optional<uint32_t> optIndex = m_oProgram.FindUniform(sSampler);
                if (!optIndex)
                {
                    return;
                }

                const ShaderUniform& oUniform = m_oProgram.GetUniforms()[*optIndex];
                if (oUniform.eType != ShaderUniformType::Sampler || oUniform.uTextureStage != oTexture.uStage)
                {
                    throw std::logic_error(std::format("Material '{}' texture '{}' expects {} to be a sampler on stage {}", m_svMaterialName, oTexture.sName, sSampler, oTexture.uStage));
                }
            }

            void VerifyNoUndeclaredTextures(const MaterialParameterLayout& oLayout) const
            {
                for (const ShaderUniform& oUniform : m_oProgram.GetUniforms())
                {
                    if (oUniform.eType != ShaderUniformType::Sampler || oUniform.bPredefined || !oUniform.sName.starts_with(MaterialShaderNames::kMaterialTexturePrefix))
                    {
                        continue;
                    }

                    const bool bDeclared = std::ranges::any_of(oLayout.vTextures, [&oUniform](const MaterialTextureParameter& oTexture) {
                        return MaterialShaderNames::GetTextureSamplerName(oTexture.uStage) == oUniform.sName;
                    });
                    if (!bDeclared)
                    {
                        throw std::logic_error(std::format("Material '{}' shader samples {} but the parameter layout declares no texture for it", m_svMaterialName, oUniform.sName));
                    }
                }
            }

            void VerifyEngineUniform(std::string_view svName, uint16_t uExpectedCount) const
            {
                const std::optional<uint32_t> optIndex = m_oProgram.FindUniform(svName);
                if (!optIndex)
                {
                    return;
                }

                const ShaderUniform& oUniform = m_oProgram.GetUniforms()[*optIndex];
                if (oUniform.eType != ShaderUniformType::Vec4 || oUniform.uCount != uExpectedCount)
                {
                    throw std::logic_error(std::format("Material '{}' shader declares {} as {}[{}] but the engine provides Vec4[{}]",
                                                       m_svMaterialName,
                                                       svName,
                                                       ShaderInterfaceText::GetTypeName(oUniform.eType),
                                                       oUniform.uCount,
                                                       uExpectedCount));
                }
            }

            std::string_view m_svMaterialName;
            const ShaderProgram& m_oProgram;
        };
    }

    struct Material::Impl
    {
        struct PermutationRecord
        {
            MaterialPermutationKey oKey;
            std::unique_ptr<ShaderProgram> upProgram;
            MaterialPermutation oPermutation;
        };

        struct SwitchSetRecord
        {
            std::vector<MaterialStaticSwitchValue> vValues;
            MaterialDescriptor oDescriptor;
            ShaderModuleSource oModule;
        };

        Impl(MaterialSystem& oMaterialSystem, MaterialDescriptor oMaterialDescriptor, std::vector<MaterialStaticSwitchValue> vDefaultSwitches)
            : pSystem(&oMaterialSystem)
        {
            for (const MaterialStaticSwitchValue& oValue : vDefaultSwitches)
            {
                vStaticSwitchNames.push_back(oValue.sName);
            }
            static_cast<void>(AddSwitchSet(std::move(vDefaultSwitches), std::move(oMaterialDescriptor)));
        }

        [[nodiscard]] const MaterialDescriptor& GetDefaultDescriptor() const noexcept
        {
            return vSwitchSets.front().oDescriptor;
        }

        [[nodiscard]] uint32_t AddSwitchSet(std::vector<MaterialStaticSwitchValue> vValues, MaterialDescriptor oDescriptor)
        {
            ValidateDescriptor(oDescriptor);
            if (oDescriptor.sDebugName.empty())
            {
                oDescriptor.sDebugName = oDescriptor.sModuleName;
            }
            const std::string sModulePath = oDescriptor.sSourcePath.empty() ? std::format("{}{}.slang", kMaterialModuleDirectory, oDescriptor.sModuleName) : oDescriptor.sSourcePath;

            SwitchSetRecord oRecord;
            oRecord.vValues = std::move(vValues);
            oRecord.oModule = {oDescriptor.sModuleName, sModulePath, oDescriptor.sSlangSource};
            oRecord.oDescriptor = std::move(oDescriptor);
            vSwitchSets.push_back(std::move(oRecord));
            return static_cast<uint32_t>(vSwitchSets.size() - 1);
        }

        [[nodiscard]] std::vector<MaterialStaticSwitchValue> MergeSwitchValues(std::span<const MaterialStaticSwitchValue> spanValues) const
        {
            std::vector<MaterialStaticSwitchValue> vMerged = vSwitchSets.front().vValues;
            for (const MaterialStaticSwitchValue& oValue : spanValues)
            {
                const auto itSwitch = std::ranges::find(vMerged, oValue.sName, &MaterialStaticSwitchValue::sName);
                if (itSwitch == vMerged.end())
                {
                    throw std::invalid_argument(std::format("Material '{}' has no static switch named '{}'", GetDefaultDescriptor().sDebugName, oValue.sName));
                }
                itSwitch->bValue = oValue.bValue;
            }
            return vMerged;
        }

        [[nodiscard]] uint32_t ResolveSwitchSet(std::span<const MaterialStaticSwitchValue> spanValues)
        {
            if (!optGraph)
            {
                if (spanValues.empty())
                {
                    return 0;
                }
                throw std::logic_error(std::format("Material '{}' is not built from a material graph and has no static switches", GetDefaultDescriptor().sDebugName));
            }

            const std::vector<MaterialStaticSwitchValue> vMerged = MergeSwitchValues(spanValues);
            const auto itExisting = std::ranges::find(vSwitchSets, vMerged, &SwitchSetRecord::vValues);
            if (itExisting != vSwitchSets.end())
            {
                return static_cast<uint32_t>(std::distance(vSwitchSets.begin(), itExisting));
            }

            MaterialCompileResult oResult = MaterialCompiler().Compile(*optGraph, vMerged);
            const uint32_t uIndex = AddSwitchSet(std::move(oResult.vStaticSwitches), std::move(oResult.oDescriptor));
            LogGenerated(vSwitchSets[uIndex]);
            return uIndex;
        }

        void LogGenerated(const SwitchSetRecord& oRecord) const
        {
            if (Core::Logger::IsInitialized())
            {
                Core::Logger::Get().debug("Generated Slang module '{}' for material graph '{}' (static switches: {}):\n{}",
                                          oRecord.oDescriptor.sModuleName,
                                          oRecord.oDescriptor.sDebugName,
                                          DescribeSwitches(oRecord.vValues),
                                          oRecord.oDescriptor.sSlangSource);
            }
        }

        [[nodiscard]] const SwitchSetRecord& RequireSwitchSet(uint32_t uSet) const
        {
            if (uSet >= vSwitchSets.size())
            {
                throw std::invalid_argument(std::format("Material '{}' has no static switch set {} ({} resolved)", GetDefaultDescriptor().sDebugName, uSet, vSwitchSets.size()));
            }
            return vSwitchSets[uSet];
        }

        [[nodiscard]] const MaterialPermutation* Find(const MaterialPermutationKey& oKey) const noexcept
        {
            for (const std::unique_ptr<PermutationRecord>& upRecord : vPermutations)
            {
                if (upRecord->oKey == oKey)
                {
                    return &upRecord->oPermutation;
                }
            }
            return nullptr;
        }

        [[nodiscard]] static SpecializedShaderRequest BuildRequest(const ShaderModuleSource& oPassModule, const SwitchSetRecord& oSet, ShaderStage eStage, const MaterialPermutationKey& oKey)
        {
            SpecializedShaderRequest oRequest;
            oRequest.oPassModule = oPassModule;
            oRequest.eStage = eStage;
            oRequest.vModules = {oSet.oModule};
            if (eStage == ShaderStage::Vertex)
            {
                oRequest.sEntryPoint = std::string(kVertexEntryPoint);
                oRequest.vTypeArguments = {std::string(VertexFactoryTable::GetTypeName(oKey.eVertexFactory)), oSet.oDescriptor.sModuleName};
            }
            else
            {
                oRequest.sEntryPoint = std::string(kFragmentEntryPoint);
                oRequest.vTypeArguments = {oSet.oDescriptor.sModuleName};
            }
            return oRequest;
        }

        [[nodiscard]] const MaterialPermutation& Compile(const MaterialPermutationKey& oKey)
        {
            const SwitchSetRecord& oSet = RequireSwitchSet(oKey.uStaticSwitchSet);
            const std::chrono::steady_clock::time_point tStart = std::chrono::steady_clock::now();

            const ShaderModuleSource oPassModule = MaterialPassTable::LoadModule(oKey.ePass);
            CachingShaderCompiler oCompiler(pSystem->GetCompiler(), pSystem->GetShaderCache());
            const CachedShaderResult oVertex = oCompiler.CompileSpecialized(BuildRequest(oPassModule, oSet, ShaderStage::Vertex, oKey));
            const CachedShaderResult oFragment = oCompiler.CompileSpecialized(BuildRequest(oPassModule, oSet, ShaderStage::Fragment, oKey));

            auto upProgram = std::make_unique<ShaderProgram>(oVertex.oShader, oFragment.oShader);
            MaterialInterfaceVerifier(oSet.oDescriptor.sDebugName, *upProgram).Verify(oSet.oDescriptor.oParameters);

            auto upRecord = std::make_unique<PermutationRecord>();
            upRecord->oKey = oKey;
            upRecord->oPermutation.uIndex = static_cast<uint32_t>(vPermutations.size());
            upRecord->oPermutation.pProgram = upProgram.get();
            upRecord->upProgram = std::move(upProgram);
            vPermutations.push_back(std::move(upRecord));

            const std::chrono::duration<double, std::milli> tElapsed = std::chrono::steady_clock::now() - tStart;
            const uint32_t uCachedStages = (oVertex.bFromCache ? 1u : 0u) + (oFragment.bFromCache ? 1u : 0u);
            LogCompiled(oKey, tElapsed.count(), CachingShaderCompiler::DescribeOrigin(uCachedStages, 2), oVertex.oShader, oFragment.oShader);
            return vPermutations.back()->oPermutation;
        }

        void LogCompiled(const MaterialPermutationKey& oKey, double dMilliseconds, std::string_view svOrigin, const CompiledShader& oVertexShader, const CompiledShader& oFragmentShader) const
        {
            if (!Core::Logger::IsInitialized())
            {
                return;
            }

            auto& oLogger = Core::Logger::Get();
            const SwitchSetRecord& oSet = vSwitchSets[oKey.uStaticSwitchSet];
            oLogger.info("Compiled material '{}' permutation {} in {:.1f} ms ({}); static switches: {}",
                         oSet.oDescriptor.sDebugName,
                         DescribeKey(oKey),
                         dMilliseconds,
                         svOrigin,
                         DescribeSwitches(oSet.vValues));
            for (const CompiledShader* pShader : {&oVertexShader, &oFragmentShader})
            {
                oLogger.debug("  {} ({} bytes): {}", pShader->sName, pShader->vBgfxBinary.size(), ShaderInterfaceText::DescribeUniforms(pShader->oInterface));
            }
        }

        MaterialSystem* pSystem;
        std::vector<SwitchSetRecord> vSwitchSets;
        std::vector<std::string> vStaticSwitchNames;
        std::vector<std::unique_ptr<PermutationRecord>> vPermutations;
        std::optional<MaterialGraph> optGraph;
    };

    Material::Material(MaterialSystem& oSystem, MaterialDescriptor oDescriptor)
        : m_upImpl(std::make_unique<Impl>(oSystem, std::move(oDescriptor), std::vector<MaterialStaticSwitchValue>{}))
    {
    }

    Material::Material(MaterialSystem& oSystem, MaterialGraph oGraph)
    {
        MaterialCompileResult oResult = MaterialCompiler().Compile(oGraph);
        m_upImpl = std::make_unique<Impl>(oSystem, std::move(oResult.oDescriptor), std::move(oResult.vStaticSwitches));
        m_upImpl->optGraph = std::move(oGraph);
        m_upImpl->LogGenerated(m_upImpl->vSwitchSets.front());
    }

    Material::~Material() = default;

    const MaterialPermutation& Material::CompilePermutation(const MaterialPermutationKey& oKey)
    {
        const MaterialPermutation* pPermutation = m_upImpl->Find(oKey);
        return pPermutation != nullptr ? *pPermutation : m_upImpl->Compile(oKey);
    }

    const MaterialParameterLayout& Material::GetParameterLayout() const noexcept
    {
        return m_upImpl->GetDefaultDescriptor().oParameters;
    }

    std::string_view Material::GetDebugName() const noexcept
    {
        return m_upImpl->GetDefaultDescriptor().sDebugName;
    }

    MaterialSystem& Material::GetSystem() const noexcept
    {
        return *m_upImpl->pSystem;
    }

    uint32_t Material::GetPermutationCount() const noexcept
    {
        return static_cast<uint32_t>(m_upImpl->vPermutations.size());
    }

    const MaterialGraph* Material::GetGraph() const noexcept
    {
        return m_upImpl->optGraph ? &*m_upImpl->optGraph : nullptr;
    }

    uint32_t Material::ResolveStaticSwitchSet(std::span<const MaterialStaticSwitchValue> spanValues)
    {
        return m_upImpl->ResolveSwitchSet(spanValues);
    }

    std::span<const std::string> Material::GetStaticSwitchNames() const noexcept
    {
        return m_upImpl->vStaticSwitchNames;
    }

    uint32_t Material::GetStaticSwitchSetCount() const noexcept
    {
        return static_cast<uint32_t>(m_upImpl->vSwitchSets.size());
    }
}
