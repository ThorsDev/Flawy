#include "Flawy/Renderer/Shader/ShaderCompiler.hpp"

#include "Flawy/Core/Logger.hpp"
#include "Flawy/Renderer/Shader/EmbeddedShaderSourceProvider.hpp"
#include "Flawy/Renderer/Shader/ShaderCompileError.hpp"
#include "Renderer/Shader/BgfxShaderBinaryWriter.hpp"
#include "Renderer/Shader/BgfxShaderInterfaceBuilder.hpp"
#include "Renderer/Shader/IShaderBackendEmitter.hpp"
#include "Renderer/Shader/ShaderBackendEmitterFactory.hpp"
#include "Renderer/Shader/ShaderSourceFileSystem.hpp"

#include <slang-com-ptr.h>
#include <slang.h>

#include <array>
#include <format>
#include <span>
#include <stdexcept>
#include <string>
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

        class CompilationContext final
        {
        public:
            CompilationContext(const ShaderSource& oSource, ShaderStage eStage, std::string_view svEntryPoint)
                : m_oSource(oSource)
                , m_eStage(eStage)
                , m_sEntryPoint(svEntryPoint)
            {
            }

            [[noreturn]] void Fail(std::string_view svReason) const
            {
                std::string sMessage = std::format("Failed to compile {} shader '{}' entry point '{}': {}", ToStageName(m_eStage), m_oSource.sName, m_sEntryPoint, svReason);
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

            [[nodiscard]] const std::string& GetEntryPoint() const noexcept
            {
                return m_sEntryPoint;
            }

            [[nodiscard]] const ShaderSource& GetSource() const noexcept
            {
                return m_oSource;
            }

            [[nodiscard]] ShaderStage GetStage() const noexcept
            {
                return m_eStage;
            }

        private:
            const ShaderSource& m_oSource;
            ShaderStage m_eStage;
            std::string m_sEntryPoint;
            DiagnosticLog m_oDiagnostics;
        };
    }

    struct ShaderCompiler::Impl
    {
        explicit Impl(RendererBackend eBackend)
            : upEmitter(ShaderBackendEmitterFactory::Create(eBackend))
            , oFileSystem(oIncludeProvider)
        {
            if (SLANG_FAILED(slang::createGlobalSession(cpGlobalSession.writeRef())))
            {
                throw std::runtime_error("Failed to create the Slang global session");
            }
        }

        [[nodiscard]] Slang::ComPtr<slang::ISession> CreateSession(CompilationContext& oContext)
        {
            std::vector<slang::CompilerOptionEntry> vOptions = {
                MakeIntOption(slang::CompilerOptionName::MatrixLayoutColumn, 1),
                MakeIntOption(slang::CompilerOptionName::Optimization, SLANG_OPTIMIZATION_LEVEL_DEFAULT),
                MakeIntOption(slang::CompilerOptionName::DebugInformation, SLANG_DEBUG_INFO_LEVEL_NONE),
            };
            upEmitter->AppendTargetOptions(oContext.GetStage(), vOptions);

            slang::TargetDesc oTarget;
            oTarget.format = upEmitter->GetTargetFormat();
            oTarget.profile = cpGlobalSession->findProfile(upEmitter->GetProfileName());
            oTarget.compilerOptionEntries = vOptions.data();
            oTarget.compilerOptionEntryCount = static_cast<uint32_t>(vOptions.size());

            std::vector<slang::PreprocessorMacroDesc> vMacros;
            for (const ShaderDefine& oDefine : oContext.GetSource().vDefines)
            {
                vMacros.push_back({oDefine.sName.c_str(), oDefine.sValue.c_str()});
            }

            slang::SessionDesc oSessionDesc;
            oSessionDesc.targets = &oTarget;
            oSessionDesc.targetCount = 1;
            oSessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;
            oSessionDesc.preprocessorMacros = vMacros.data();
            oSessionDesc.preprocessorMacroCount = static_cast<SlangInt>(vMacros.size());
            oSessionDesc.fileSystem = &oFileSystem;
            oSessionDesc.searchPaths = kIncludeSearchPaths.data();
            oSessionDesc.searchPathCount = static_cast<SlangInt>(kIncludeSearchPaths.size());

            Slang::ComPtr<slang::ISession> cpSession;
            oContext.Check(cpGlobalSession->createSession(oSessionDesc, cpSession.writeRef()), "could not create a Slang session");
            return cpSession;
        }

        [[nodiscard]] Slang::ComPtr<slang::IComponentType> LinkProgram(slang::ISession& oSession, CompilationContext& oContext) const
        {
            const ShaderSource& oSource = oContext.GetSource();

            Slang::ComPtr<slang::IBlob> cpModuleDiagnostics;
            slang::IModule* pModule = oSession.loadModuleFromSourceString(oSource.sName.c_str(), oSource.sPath.c_str(), oSource.sCode.c_str(), cpModuleDiagnostics.writeRef());
            oContext.GetDiagnostics().Append(cpModuleDiagnostics.get());
            if (pModule == nullptr)
            {
                oContext.Fail("the module could not be loaded");
            }

            Slang::ComPtr<slang::IEntryPoint> cpEntryPoint;
            if (SLANG_FAILED(pModule->findEntryPointByName(oContext.GetEntryPoint().c_str(), cpEntryPoint.writeRef())) || !cpEntryPoint)
            {
                oContext.Fail("entry point not found; it must be marked with [shader(\"...\")]");
            }

            const std::array<slang::IComponentType*, 2> aComponents = {pModule, cpEntryPoint.get()};
            Slang::ComPtr<slang::IComponentType> cpComposite;
            Slang::ComPtr<slang::IBlob> cpComposeDiagnostics;
            const SlangResult iComposeResult = oSession.createCompositeComponentType(aComponents.data(), static_cast<SlangInt>(aComponents.size()), cpComposite.writeRef(), cpComposeDiagnostics.writeRef());
            oContext.GetDiagnostics().Append(cpComposeDiagnostics.get());
            oContext.Check(iComposeResult, "could not compose the program");

            Slang::ComPtr<slang::IComponentType> cpLinked;
            Slang::ComPtr<slang::IBlob> cpLinkDiagnostics;
            const SlangResult iLinkResult = cpComposite->link(cpLinked.writeRef(), cpLinkDiagnostics.writeRef());
            oContext.GetDiagnostics().Append(cpLinkDiagnostics.get());
            oContext.Check(iLinkResult, "linking failed");
            return cpLinked;
        }

        [[nodiscard]] CompiledShader Compile(CompilationContext& oContext)
        {
            const Slang::ComPtr<slang::ISession> cpSession = CreateSession(oContext);
            const Slang::ComPtr<slang::IComponentType> cpProgram = LinkProgram(*cpSession.get(), oContext);

            Slang::ComPtr<slang::IBlob> cpLayoutDiagnostics;
            slang::ProgramLayout* pLayout = cpProgram->getLayout(0, cpLayoutDiagnostics.writeRef());
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
            const SlangResult iCodeResult = cpProgram->getEntryPointCode(0, 0, cpCode.writeRef(), cpCodeDiagnostics.writeRef());
            oContext.GetDiagnostics().Append(cpCodeDiagnostics.get());
            oContext.Check(iCodeResult, "code generation failed");

            Slang::ComPtr<slang::IMetadata> cpMetadata;
            Slang::ComPtr<slang::IBlob> cpMetadataDiagnostics;
            const SlangResult iMetadataResult = cpProgram->getEntryPointMetadata(0, 0, cpMetadata.writeRef(), cpMetadataDiagnostics.writeRef());
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
            oShader.sName = oContext.GetSource().sName;
            oShader.sEntryPoint = oContext.GetEntryPoint();
            oShader.vBgfxBinary = BgfxShaderBinaryWriter().Write(oContext.GetStage(), oInterface, vShaderCode);
            oShader.oInterface = BgfxShaderInterfaceBuilder::ToPublicInterface(oInterface);
            return oShader;
        }

        std::unique_ptr<IShaderBackendEmitter> upEmitter;
        EmbeddedShaderSourceProvider oIncludeProvider;
        ShaderSourceFileSystem oFileSystem;
        Slang::ComPtr<slang::IGlobalSession> cpGlobalSession;
    };

    ShaderCompiler::ShaderCompiler(RendererBackend eBackend)
        : m_upImpl(std::make_unique<Impl>(eBackend))
    {
    }

    ShaderCompiler::~ShaderCompiler() = default;

    CompiledShader ShaderCompiler::Compile(const ShaderSource& oSource, ShaderStage eStage, std::string_view svEntryPoint)
    {
        CompilationContext oContext(oSource, eStage, svEntryPoint);
        CompiledShader oShader = m_upImpl->Compile(oContext);

        const std::string& sWarnings = oContext.GetDiagnostics().GetText();
        if (!sWarnings.empty() && Core::Logger::IsInitialized())
        {
            Core::Logger::Get().warn("Shader '{}' ({}) compiled with diagnostics:\n{}", oSource.sName, svEntryPoint, sWarnings);
        }
        return oShader;
    }
}
