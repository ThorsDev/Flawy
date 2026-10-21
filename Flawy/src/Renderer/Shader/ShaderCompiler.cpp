#include "Flawy/Renderer/Shader/ShaderCompiler.hpp"

#include "Flawy/Core/Logger.hpp"
#include "Flawy/Renderer/Shader/EmbeddedShaderSourceProvider.hpp"
#include "Flawy/Renderer/Shader/ShaderCompileError.hpp"
#include "Renderer/Shader/BgfxShaderBinaryWriter.hpp"
#include "Renderer/Shader/BgfxShaderInterfaceBuilder.hpp"
#include "Renderer/Shader/EmbeddedShaderTable.hpp"
#include "Renderer/Shader/IShaderBackendEmitter.hpp"
#include "Renderer/Shader/ShaderBackendEmitterFactory.hpp"
#include "Renderer/Shader/ShaderSourceFileSystem.hpp"

#include <slang-com-ptr.h>
#include <slang-tag-version.h>
#include <slang.h>

#include <array>
#include <cstdint>
#include <format>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace Flawy::Renderer
{
    namespace
    {
        class DiagnosticLog final
        {
        public:
            void Append(slang::IBlob* pDiagnostics)
            {
                if (pDiagnostics == nullptr || pDiagnostics->getBufferSize() == 0)
                {
                    return;
                }

                std::string_view svText(static_cast<const char*>(pDiagnostics->getBufferPointer()), pDiagnostics->getBufferSize());
                while (!svText.empty() && (svText.back() == '\0' || svText.back() == '\n' || svText.back() == '\r'))
                {
                    svText.remove_suffix(1);
                }
                if (svText.empty())
                {
                    return;
                }

                if (!m_sText.empty())
                {
                    m_sText.push_back('\n');
                }
                m_sText.append(svText);
            }

            [[nodiscard]] const std::string& GetText() const noexcept
            {
                return m_sText;
            }

        private:
            std::string m_sText;
        };

        SlangStage ToSlangStage(ShaderStage eStage) noexcept
        {
            switch (eStage)
            {
            case ShaderStage::Vertex:
                return SLANG_STAGE_VERTEX;
            case ShaderStage::Fragment:
                return SLANG_STAGE_FRAGMENT;
            case ShaderStage::Compute:
                return SLANG_STAGE_COMPUTE;
            }
            return SLANG_STAGE_NONE;
        }

        std::string_view ToStageName(ShaderStage eStage) noexcept
        {
            switch (eStage)
            {
            case ShaderStage::Vertex:
                return "vertex";
            case ShaderStage::Fragment:
                return "fragment";
            case ShaderStage::Compute:
                return "compute";
            }
            return "unknown";
        }

        slang::CompilerOptionEntry MakeIntOption(slang::CompilerOptionName eName, int32_t iValue) noexcept
        {
            slang::CompilerOptionEntry oEntry;
            oEntry.name = eName;
            oEntry.value.kind = slang::CompilerOptionValueKind::Int;
            oEntry.value.intValue0 = iValue;
            return oEntry;
        }

        constexpr std::array<const char*, 1> kIncludeSearchPaths = {""};
        constexpr uint32_t kCompilerPipelineVersion = 1;

        class CompilerFingerprintBuilder final
        {
        public:
            void AppendField(std::string_view svKey, std::string_view svValue)
            {
                m_sText.append(std::format("{}={}:{};", svKey, svValue.size(), svValue));
            }

            void AppendInteger(std::string_view svKey, int64_t iValue)
            {
                AppendField(svKey, std::to_string(iValue));
            }

            void AppendTarget(std::string_view svStage, const slang::TargetDesc& oTarget)
            {
                AppendInteger(std::format("{}.format", svStage), static_cast<int64_t>(oTarget.format));
                AppendInteger(std::format("{}.profile", svStage), static_cast<int64_t>(oTarget.profile));
                for (uint32_t uIndex = 0; uIndex < oTarget.compilerOptionEntryCount; ++uIndex)
                {
                    const slang::CompilerOptionEntry& oEntry = oTarget.compilerOptionEntries[uIndex];
                    const std::string sKey = std::format("{}.option{}", svStage, static_cast<int32_t>(oEntry.name));
                    AppendInteger(sKey + ".kind", static_cast<int64_t>(oEntry.value.kind));
                    AppendInteger(sKey + ".int0", oEntry.value.intValue0);
                    AppendInteger(sKey + ".int1", oEntry.value.intValue1);
                    AppendField(sKey + ".string0", oEntry.value.stringValue0 != nullptr ? std::string_view(oEntry.value.stringValue0) : std::string_view());
                    AppendField(sKey + ".string1", oEntry.value.stringValue1 != nullptr ? std::string_view(oEntry.value.stringValue1) : std::string_view());
                }
            }

            void AppendBindingRules(std::string_view svStage, const BgfxBindingRules& oRules)
            {
                AppendField(std::format("{}.bindings", svStage),
                            std::format("{},{},{},{},{},{},{}",
                                        oRules.uUniformBlockBinding,
                                        oRules.uTextureBindingBase,
                                        oRules.uSamplerBindingOffset,
                                        static_cast<int32_t>(oRules.eUniformBlockCategory),
                                        static_cast<int32_t>(oRules.eTextureCategory),
                                        static_cast<int32_t>(oRules.eSamplerCategory),
                                        oRules.bReflectionReportsUniformBlockBinding));
            }

            void AppendEmbeddedLibrary()
            {
                for (const EmbeddedShaderEntry& oEntry : EmbeddedShaderTable::GetEntries())
                {
                    AppendField("library.path", oEntry.szPath);
                    AppendField("library.code", std::string_view(reinterpret_cast<const char*>(oEntry.pData), oEntry.uSize));
                }
            }

            [[nodiscard]] std::string Release() noexcept
            {
                return std::move(m_sText);
            }

        private:
            std::string m_sText;
        };

        class CompilationContext final
        {
        public:
            CompilationContext(std::string sShaderName, ShaderStage eStage, std::string_view svEntryPoint)
                : m_sShaderName(std::move(sShaderName))
                , m_eStage(eStage)
                , m_sEntryPoint(svEntryPoint)
            {
            }

            [[noreturn]] void Fail(std::string_view svReason) const
            {
                std::string sMessage = std::format("Failed to compile {} shader '{}' entry point '{}': {}", ToStageName(m_eStage), m_sShaderName, m_sEntryPoint, svReason);
                if (!m_oDiagnostics.GetText().empty())
                {
                    sMessage.append("\n");
                    sMessage.append(m_oDiagnostics.GetText());
                }
                throw ShaderCompileError(sMessage);
            }

            void Check(SlangResult iResult, std::string_view svStep) const
            {
                if (SLANG_FAILED(iResult))
                {
                    Fail(svStep);
                }
            }

            [[nodiscard]] DiagnosticLog& GetDiagnostics() noexcept
            {
                return m_oDiagnostics;
            }

            [[nodiscard]] const std::string& GetShaderName() const noexcept
            {
                return m_sShaderName;
            }

            [[nodiscard]] const std::string& GetEntryPoint() const noexcept
            {
                return m_sEntryPoint;
            }

            [[nodiscard]] ShaderStage GetStage() const noexcept
            {
                return m_eStage;
            }

            void LogWarnings() const
            {
                const std::string& sWarnings = m_oDiagnostics.GetText();
                if (!sWarnings.empty() && Core::Logger::IsInitialized())
                {
                    Core::Logger::Get().warn("Shader '{}' ({}) compiled with diagnostics:\n{}", m_sShaderName, m_sEntryPoint, sWarnings);
                }
            }

        private:
            std::string m_sShaderName;
            ShaderStage m_eStage;
            std::string m_sEntryPoint;
            DiagnosticLog m_oDiagnostics;
        };

        std::string BuildSpecializedShaderName(const SpecializedShaderRequest& oRequest)
        {
            std::string sName = std::format("{}::{}", oRequest.oPassModule.sName, oRequest.sEntryPoint);
            if (oRequest.vTypeArguments.empty())
            {
                return sName;
            }

            sName.push_back('<');
            for (size_t uIndex = 0; uIndex < oRequest.vTypeArguments.size(); ++uIndex)
            {
                if (uIndex != 0)
                {
                    sName.push_back(',');
                }
                sName.append(oRequest.vTypeArguments[uIndex]);
            }
            sName.push_back('>');
            return sName;
        }

        SlangInt GetSpecializationTargetIndex(ShaderStage eStage)
        {
            switch (eStage)
            {
            case ShaderStage::Vertex:
                return 0;
            case ShaderStage::Fragment:
                return 1;
            case ShaderStage::Compute:
                break;
            }
            throw std::invalid_argument("Specialized shaders support only the vertex and fragment stages");
        }

        void RequireModuleDeclaration(const ShaderModuleSource& oModule)
        {
            if (oModule.sName.empty() || oModule.sCode.find(std::format("module {};", oModule.sName)) == std::string::npos)
            {
                throw std::invalid_argument(std::format("Shader module '{}' ({}) must declare 'module {};'", oModule.sName, oModule.sPath, oModule.sName));
            }
        }
    }

    struct ShaderCompiler::Impl
    {
        struct ModuleRecord
        {
            Slang::ComPtr<slang::IModule> cpModule;
            std::string sCode;
        };

        explicit Impl(RendererBackend eBackend)
            : upEmitter(ShaderBackendEmitterFactory::Create(eBackend))
            , oFileSystem(oIncludeProvider)
        {
            if (SLANG_FAILED(slang::createGlobalSession(cpGlobalSession.writeRef())))
            {
                throw std::runtime_error("Failed to create the Slang global session");
            }
            sCacheFingerprint = BuildCacheFingerprint(eBackend);
        }

        [[nodiscard]] std::string BuildCacheFingerprint(RendererBackend eBackend) const
        {
            CompilerFingerprintBuilder oBuilder;
            oBuilder.AppendInteger("pipeline", kCompilerPipelineVersion);
            oBuilder.AppendField("slang.header", SLANG_TAG_VERSION);
            const char* szBuildTag = cpGlobalSession->getBuildTagString();
            oBuilder.AppendField("slang.runtime", szBuildTag != nullptr ? std::string_view(szBuildTag) : std::string_view());
            oBuilder.AppendInteger("bgfx.binary", BgfxShaderBinaryWriter::kFormatVersion);
            oBuilder.AppendInteger("backend", static_cast<int64_t>(eBackend));
            oBuilder.AppendField("profile", upEmitter->GetProfileName());
            for (const ShaderStage eStage : {ShaderStage::Vertex, ShaderStage::Fragment})
            {
                std::vector<slang::CompilerOptionEntry> vOptions = BuildTargetOptions(eStage);
                oBuilder.AppendTarget(ToStageName(eStage), BuildTarget(vOptions));
                oBuilder.AppendBindingRules(ToStageName(eStage), upEmitter->GetBindingRules(eStage));
            }
            oBuilder.AppendInteger("session.matrixLayout", SLANG_MATRIX_LAYOUT_COLUMN_MAJOR);
            oBuilder.AppendEmbeddedLibrary();
            return oBuilder.Release();
        }

        [[nodiscard]] std::vector<slang::CompilerOptionEntry> BuildTargetOptions(ShaderStage eStage) const
        {
            std::vector<slang::CompilerOptionEntry> vOptions = {
                MakeIntOption(slang::CompilerOptionName::MatrixLayoutColumn, 1),
                MakeIntOption(slang::CompilerOptionName::Optimization, SLANG_OPTIMIZATION_LEVEL_DEFAULT),
                MakeIntOption(slang::CompilerOptionName::DebugInformation, SLANG_DEBUG_INFO_LEVEL_NONE),
            };
            upEmitter->AppendTargetOptions(eStage, vOptions);
            return vOptions;
        }

        [[nodiscard]] slang::TargetDesc BuildTarget(std::vector<slang::CompilerOptionEntry>& vOptions) const
        {
            slang::TargetDesc oTarget;
            oTarget.format = upEmitter->GetTargetFormat();
            oTarget.profile = cpGlobalSession->findProfile(upEmitter->GetProfileName());
            oTarget.compilerOptionEntries = vOptions.data();
            oTarget.compilerOptionEntryCount = static_cast<uint32_t>(vOptions.size());
            return oTarget;
        }

        [[nodiscard]] slang::SessionDesc BuildSessionDescription(std::span<const slang::TargetDesc> spanTargets, std::span<const slang::PreprocessorMacroDesc> spanMacros)
        {
            slang::SessionDesc oSessionDesc;
            oSessionDesc.targets = spanTargets.data();
            oSessionDesc.targetCount = static_cast<SlangInt>(spanTargets.size());
            oSessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
            oSessionDesc.preprocessorMacros = spanMacros.data();
            oSessionDesc.preprocessorMacroCount = static_cast<SlangInt>(spanMacros.size());
            oSessionDesc.fileSystem = &oFileSystem;
            oSessionDesc.searchPaths = kIncludeSearchPaths.data();
            oSessionDesc.searchPathCount = static_cast<SlangInt>(kIncludeSearchPaths.size());
            return oSessionDesc;
        }

        [[nodiscard]] Slang::ComPtr<slang::ISession> CreateSession(const ShaderSource& oSource, CompilationContext& oContext)
        {
            std::vector<slang::CompilerOptionEntry> vOptions = BuildTargetOptions(oContext.GetStage());
            const std::array<slang::TargetDesc, 1> aTargets = {BuildTarget(vOptions)};

            std::vector<slang::PreprocessorMacroDesc> vMacros;
            for (const ShaderDefine& oDefine : oSource.vDefines)
            {
                vMacros.push_back({oDefine.sName.c_str(), oDefine.sValue.c_str()});
            }

            const slang::SessionDesc oSessionDesc = BuildSessionDescription(aTargets, vMacros);
            Slang::ComPtr<slang::ISession> cpSession;
            oContext.Check(cpGlobalSession->createSession(oSessionDesc, cpSession.writeRef()), "could not create a Slang session");
            return cpSession;
        }

        [[nodiscard]] slang::ISession& RequireSpecializationSession(CompilationContext& oContext)
        {
            if (cpSpecializationSession)
            {
                return *cpSpecializationSession.get();
            }

            std::vector<slang::CompilerOptionEntry> vVertexOptions = BuildTargetOptions(ShaderStage::Vertex);
            std::vector<slang::CompilerOptionEntry> vFragmentOptions = BuildTargetOptions(ShaderStage::Fragment);
            const std::array<slang::TargetDesc, 2> aTargets = {BuildTarget(vVertexOptions), BuildTarget(vFragmentOptions)};

            const slang::SessionDesc oSessionDesc = BuildSessionDescription(aTargets, {});
            oContext.Check(cpGlobalSession->createSession(oSessionDesc, cpSpecializationSession.writeRef()), "could not create the Slang specialization session");
            return *cpSpecializationSession.get();
        }

        [[nodiscard]] slang::IModule& RegisterModule(slang::ISession& oSession, const ShaderModuleSource& oModule, CompilationContext& oContext)
        {
            RequireModuleDeclaration(oModule);

            const auto itRecord = mapModules.find(oModule.sName);
            if (itRecord != mapModules.end())
            {
                if (itRecord->second.sCode != oModule.sCode)
                {
                    throw std::invalid_argument(std::format("Shader module '{}' is already loaded with different source", oModule.sName));
                }
                return *itRecord->second.cpModule.get();
            }

            Slang::ComPtr<slang::IBlob> cpDiagnostics;
            Slang::ComPtr<slang::IModule> cpModule(oSession.loadModuleFromSourceString(oModule.sName.c_str(), oModule.sPath.c_str(), oModule.sCode.c_str(), cpDiagnostics.writeRef()));
            oContext.GetDiagnostics().Append(cpDiagnostics.get());
            if (!cpModule)
            {
                oContext.Fail(std::format("module '{}' could not be loaded", oModule.sName));
            }

            slang::IModule& oLoaded = *cpModule.get();
            mapModules.emplace(oModule.sName, ModuleRecord{std::move(cpModule), oModule.sCode});
            return oLoaded;
        }

        [[nodiscard]] static Slang::ComPtr<slang::IComponentType> Compose(slang::ISession& oSession, std::span<slang::IComponentType* const> spanComponents, CompilationContext& oContext)
        {
            Slang::ComPtr<slang::IComponentType> cpComposite;
            Slang::ComPtr<slang::IBlob> cpDiagnostics;
            const SlangResult iResult = oSession.createCompositeComponentType(spanComponents.data(), static_cast<SlangInt>(spanComponents.size()), cpComposite.writeRef(), cpDiagnostics.writeRef());
            oContext.GetDiagnostics().Append(cpDiagnostics.get());
            oContext.Check(iResult, "could not compose the program");
            return cpComposite;
        }

        [[nodiscard]] static Slang::ComPtr<slang::IComponentType> Link(slang::IComponentType& oProgram, CompilationContext& oContext)
        {
            Slang::ComPtr<slang::IComponentType> cpLinked;
            Slang::ComPtr<slang::IBlob> cpDiagnostics;
            const SlangResult iResult = oProgram.link(cpLinked.writeRef(), cpDiagnostics.writeRef());
            oContext.GetDiagnostics().Append(cpDiagnostics.get());
            oContext.Check(iResult, "linking failed");
            return cpLinked;
        }

        [[nodiscard]] static Slang::ComPtr<slang::IEntryPoint> FindEntryPoint(slang::IModule& oModule, CompilationContext& oContext)
        {
            Slang::ComPtr<slang::IEntryPoint> cpEntryPoint;
            if (SLANG_FAILED(oModule.findEntryPointByName(oContext.GetEntryPoint().c_str(), cpEntryPoint.writeRef())) || !cpEntryPoint)
            {
                oContext.Fail("entry point not found; it must be marked with [shader(\"...\")]");
            }
            return cpEntryPoint;
        }

        [[nodiscard]] static Slang::ComPtr<slang::IComponentType> LinkProgram(slang::ISession& oSession, const ShaderSource& oSource, CompilationContext& oContext)
        {
            Slang::ComPtr<slang::IBlob> cpModuleDiagnostics;
            slang::IModule* pModule = oSession.loadModuleFromSourceString(oSource.sName.c_str(), oSource.sPath.c_str(), oSource.sCode.c_str(), cpModuleDiagnostics.writeRef());
            oContext.GetDiagnostics().Append(cpModuleDiagnostics.get());
            if (pModule == nullptr)
            {
                oContext.Fail("the module could not be loaded");
            }

            const Slang::ComPtr<slang::IEntryPoint> cpEntryPoint = FindEntryPoint(*pModule, oContext);
            const std::array<slang::IComponentType*, 2> aComponents = {pModule, cpEntryPoint.get()};
            const Slang::ComPtr<slang::IComponentType> cpComposite = Compose(oSession, aComponents, oContext);
            return Link(*cpComposite.get(), oContext);
        }

        [[nodiscard]] static Slang::ComPtr<slang::IComponentType> Specialize(slang::IComponentType& oComposite, const std::vector<std::string>& vTypeArguments, CompilationContext& oContext)
        {
            const SlangInt iParameterCount = oComposite.getSpecializationParamCount();
            if (iParameterCount != static_cast<SlangInt>(vTypeArguments.size()))
            {
                oContext.Fail(std::format("the entry point expects {} specialization type(s) but {} were given", iParameterCount, vTypeArguments.size()));
            }
            if (vTypeArguments.empty())
            {
                return Slang::ComPtr<slang::IComponentType>(&oComposite);
            }

            Slang::ComPtr<slang::IBlob> cpLayoutDiagnostics;
            slang::ProgramLayout* pLayout = oComposite.getLayout(0, cpLayoutDiagnostics.writeRef());
            oContext.GetDiagnostics().Append(cpLayoutDiagnostics.get());
            if (pLayout == nullptr)
            {
                oContext.Fail("no reflection layout is available for specialization");
            }

            std::vector<slang::SpecializationArg> vArguments;
            vArguments.reserve(vTypeArguments.size());
            for (const std::string& sTypeName : vTypeArguments)
            {
                slang::TypeReflection* pType = pLayout->findTypeByName(sTypeName.c_str());
                if (pType == nullptr)
                {
                    oContext.Fail(std::format("Specialization type '{}' was not found in the loaded modules", sTypeName));
                }
                vArguments.push_back(slang::SpecializationArg::fromType(pType));
            }

            Slang::ComPtr<slang::IComponentType> cpSpecialized;
            Slang::ComPtr<slang::IBlob> cpDiagnostics;
            const SlangResult iResult = oComposite.specialize(vArguments.data(), static_cast<SlangInt>(vArguments.size()), cpSpecialized.writeRef(), cpDiagnostics.writeRef());
            oContext.GetDiagnostics().Append(cpDiagnostics.get());
            if (SLANG_FAILED(iResult) || !cpSpecialized)
            {
                oContext.Fail("specialization failed");
            }
            return cpSpecialized;
        }

        [[nodiscard]] CompiledShader EmitCompiledShader(slang::IComponentType& oProgram, SlangInt iTargetIndex, CompilationContext& oContext) const
        {
            Slang::ComPtr<slang::IBlob> cpLayoutDiagnostics;
            slang::ProgramLayout* pLayout = oProgram.getLayout(iTargetIndex, cpLayoutDiagnostics.writeRef());
            oContext.GetDiagnostics().Append(cpLayoutDiagnostics.get());
            if (pLayout == nullptr || pLayout->getEntryPointCount() == 0)
            {
                oContext.Fail("no reflection layout is available");
            }

            slang::EntryPointReflection* pEntryPoint = pLayout->getEntryPointByIndex(0);
            if (pEntryPoint->getStage() != ToSlangStage(oContext.GetStage()))
            {
                oContext.Fail(std::format("the entry point is not a {} shader", ToStageName(oContext.GetStage())));
            }

            Slang::ComPtr<slang::IBlob> cpCode;
            Slang::ComPtr<slang::IBlob> cpCodeDiagnostics;
            const SlangResult iCodeResult = oProgram.getEntryPointCode(0, iTargetIndex, cpCode.writeRef(), cpCodeDiagnostics.writeRef());
            oContext.GetDiagnostics().Append(cpCodeDiagnostics.get());
            oContext.Check(iCodeResult, "code generation failed");

            Slang::ComPtr<slang::IMetadata> cpMetadata;
            Slang::ComPtr<slang::IBlob> cpMetadataDiagnostics;
            const SlangResult iMetadataResult = oProgram.getEntryPointMetadata(0, iTargetIndex, cpMetadata.writeRef(), cpMetadataDiagnostics.writeRef());
            oContext.GetDiagnostics().Append(cpMetadataDiagnostics.get());
            if (SLANG_FAILED(iMetadataResult))
            {
                cpMetadata = nullptr;
            }

            const BgfxBindingRules oRules = upEmitter->GetBindingRules(oContext.GetStage());
            const BgfxShaderInterface oInterface = BgfxShaderInterfaceBuilder().Build(*pLayout, *pEntryPoint, cpMetadata.get(), oContext.GetStage(), oRules);

            const std::span<const std::byte> spanTargetCode(static_cast<const std::byte*>(cpCode->getBufferPointer()), cpCode->getBufferSize());
            const std::vector<std::byte> vShaderCode = upEmitter->EmitShaderCode(spanTargetCode, oContext.GetStage());

            CompiledShader oShader;
            oShader.eStage = oContext.GetStage();
            oShader.sName = oContext.GetShaderName();
            oShader.sEntryPoint = oContext.GetEntryPoint();
            oShader.vBgfxBinary = BgfxShaderBinaryWriter().Write(oContext.GetStage(), oInterface, vShaderCode);
            oShader.oInterface = BgfxShaderInterfaceBuilder::ToPublicInterface(oInterface);
            return oShader;
        }

        [[nodiscard]] CompiledShader Compile(const ShaderSource& oSource, CompilationContext& oContext)
        {
            const Slang::ComPtr<slang::ISession> cpSession = CreateSession(oSource, oContext);
            const Slang::ComPtr<slang::IComponentType> cpProgram = LinkProgram(*cpSession.get(), oSource, oContext);
            return EmitCompiledShader(*cpProgram.get(), 0, oContext);
        }

        [[nodiscard]] CompiledShader CompileSpecialized(const SpecializedShaderRequest& oRequest, CompilationContext& oContext)
        {
            const SlangInt iTargetIndex = GetSpecializationTargetIndex(oRequest.eStage);
            slang::ISession& oSession = RequireSpecializationSession(oContext);

            slang::IModule& oPassModule = RegisterModule(oSession, oRequest.oPassModule, oContext);
            const Slang::ComPtr<slang::IEntryPoint> cpEntryPoint = FindEntryPoint(oPassModule, oContext);

            std::vector<slang::IComponentType*> vComponents = {&oPassModule, cpEntryPoint.get()};
            for (const ShaderModuleSource& oModule : oRequest.vModules)
            {
                vComponents.push_back(&RegisterModule(oSession, oModule, oContext));
            }

            const Slang::ComPtr<slang::IComponentType> cpComposite = Compose(oSession, vComponents, oContext);
            const Slang::ComPtr<slang::IComponentType> cpSpecialized = Specialize(*cpComposite.get(), oRequest.vTypeArguments, oContext);
            const Slang::ComPtr<slang::IComponentType> cpLinked = Link(*cpSpecialized.get(), oContext);
            return EmitCompiledShader(*cpLinked.get(), iTargetIndex, oContext);
        }

        std::unique_ptr<IShaderBackendEmitter> upEmitter;
        EmbeddedShaderSourceProvider oIncludeProvider;
        ShaderSourceFileSystem oFileSystem;
        Slang::ComPtr<slang::IGlobalSession> cpGlobalSession;
        Slang::ComPtr<slang::ISession> cpSpecializationSession;
        std::unordered_map<std::string, ModuleRecord> mapModules;
        std::string sCacheFingerprint;
    };

    ShaderCompiler::ShaderCompiler(RendererBackend eBackend)
        : m_upImpl(std::make_unique<Impl>(eBackend))
    {
    }

    ShaderCompiler::~ShaderCompiler() = default;

    CompiledShader ShaderCompiler::Compile(const ShaderSource& oSource, ShaderStage eStage, std::string_view svEntryPoint)
    {
        CompilationContext oContext(oSource.sName, eStage, svEntryPoint);
        CompiledShader oShader = m_upImpl->Compile(oSource, oContext);
        oContext.LogWarnings();
        return oShader;
    }

    CompiledShader ShaderCompiler::CompileSpecialized(const SpecializedShaderRequest& oRequest)
    {
        CompilationContext oContext(BuildSpecializedShaderName(oRequest), oRequest.eStage, oRequest.sEntryPoint);
        CompiledShader oShader = m_upImpl->CompileSpecialized(oRequest, oContext);
        oContext.LogWarnings();
        return oShader;
    }

    std::string_view ShaderCompiler::GetCacheFingerprint() const noexcept
    {
        return m_upImpl->sCacheFingerprint;
    }
}
