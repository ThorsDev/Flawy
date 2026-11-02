#include "SandboxApplication.hpp"

#include "AnimationFactory.hpp"
#include "ImportFactory.hpp"
#include "PostProcessFactory.hpp"
#include "SceneFactory.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace Sandbox
{
    namespace
    {
        constexpr std::string_view kSceneColorName = "SceneColor";
        constexpr float kVignetteStrength = 0.6f;
        constexpr glm::vec4 kPostProcessTint = {1.0f, 1.0f, 1.0f, kVignetteStrength};
        constexpr uint32_t kScreenshotLeadFrames = 10;
        constexpr uint32_t kDefaultScreenshotFrame = 60;

        Flawy::Core::ApplicationSpecification BuildSpecification(const SandboxOptions& oOptions)
        {
            Flawy::Core::ApplicationSpecification oSpecification;
            oSpecification.sName = "Flawy-Sandbox";
            oSpecification.oWindow = {.sTitle = "Flawy-Sandbox", .uWidth = 1280, .uHeight = 720, .bResizable = true};
            oSpecification.eRendererBackend = oOptions.eBackend;
            oSpecification.optShaderCacheDirectory = oOptions.optShaderCacheDirectory;
            oSpecification.vShaderSearchRoots = {std::filesystem::path(FLAWY_SANDBOX_SHADER_DIR)};
            oSpecification.optAssetRoot = oOptions.optAssetRoot.value_or(std::filesystem::path(FLAWY_SANDBOX_ASSET_DIR));
            return oSpecification;
        }
    }

    SandboxApplication::SandboxApplication(SandboxOptions oOptions)
        : Application(BuildSpecification(oOptions))
        , m_oOptions(std::move(oOptions))
        , m_oDepthPrePass(m_oScene, m_oImport)
        , m_oScenePass(m_oScene, m_oImport)
        , m_oPostProcessPass(m_oPostProcess)
    {
        m_oSceneColorDescriptor.sDebugName = std::string(kSceneColorName);
        m_oSceneColorDescriptor.eColorFormat = Flawy::Renderer::TextureFormat::Rgba16F;
    }

    void SandboxApplication::OnStart()
    {
        auto& oLogger = Flawy::Core::Logger::Get();
        oLogger.info("Flawy Engine {}", Flawy::Core::BuildInfo::GetEngineVersion());

        for (const auto& oDependency : Flawy::Core::BuildInfo::GetDependencyVersions())
        {
            oLogger.info("  {:<14} {}", oDependency.sName, oDependency.sVersion);
        }

        const auto& oRenderDevice = GetRenderDevice();
        oLogger.info("Renderer backend: {} ({}x{})", oRenderDevice.GetBackendName(), oRenderDevice.GetWidth(), oRenderDevice.GetHeight());

        Flawy::Renderer::MaterialSystem& oMaterialSystem = GetMaterialSystem();
        SceneFactory::Create(m_oScene, oMaterialSystem, GetAssetManager());
        AnimationFactory::Create(m_oAnimation, GetAssetManager());
        ImportFactory::Create(m_oImport, GetAssetManager(), m_oOptions.optImportPath);
        PostProcessFactory::Create(m_oPostProcess, GetShaderLibrary());
    }

    void SandboxApplication::OnUpdate(double dDeltaSeconds)
    {
        ++m_uFrameIndex;
        m_dElapsedSeconds += dDeltaSeconds;
        m_oAnimationDriver.Update(m_oAnimation, m_dElapsedSeconds, dDeltaSeconds);
        ImportFactory::Update(m_oImport, dDeltaSeconds);

        if (m_oOptions.optScreenshotPath && m_uFrameIndex == GetScreenshotFrame())
        {
            const std::u8string sPath = m_oOptions.optScreenshotPath->u8string();
            Flawy::Core::Logger::Get().info("Requesting screenshot {} at frame {}", reinterpret_cast<const char*>(sPath.c_str()), m_uFrameIndex);
            GetRenderDevice().RequestScreenshot(*m_oOptions.optScreenshotPath);
        }
        if (m_oOptions.uFrameLimit != 0 && m_uFrameIndex >= m_oOptions.uFrameLimit)
        {
            Flawy::Core::Logger::Get().info("Frame limit {} reached", m_oOptions.uFrameLimit);
            RequestExit();
        }
    }

    void SandboxApplication::OnRender(Flawy::Renderer::RenderGraph& oGraph)
    {
        const auto fTimeSeconds = static_cast<float>(m_dElapsedSeconds);
        const Flawy::Renderer::RenderGraphResource oSceneColor = oGraph.CreateTransientTarget(m_oSceneColorDescriptor);
        const std::span<const glm::mat4> spanJointMatrices = m_oAnimation.upAnimator->GetModelPose().GetJointMatrices();
        m_oDepthPrePass.SetJointMatrices(spanJointMatrices);
        m_oScenePass.SetJointMatrices(spanJointMatrices);
        m_oDepthPrePass.SetTimeSeconds(fTimeSeconds);
        m_oDepthPrePass.SetTarget(oSceneColor);
        oGraph.AddPass(m_oDepthPrePass);

        m_oScenePass.SetTimeSeconds(fTimeSeconds);
        m_oScenePass.SetTarget(oSceneColor);
        oGraph.AddPass(m_oScenePass);

        m_oPostProcessPass.SetTint(kPostProcessTint);
        m_oPostProcessPass.SetSource(oSceneColor);
        m_oPostProcessPass.SetTarget(oGraph.GetBackbuffer());
        oGraph.AddPass(m_oPostProcessPass);
    }

    void SandboxApplication::OnShutdown()
    {
        ImportFactory::Release(m_oImport);
        AnimationFactory::Release(m_oAnimation);
        m_oPostProcess.upParameters.reset();
        m_oPostProcess.upIndexBuffer.reset();
        m_oPostProcess.upVertexBuffer.reset();
        m_oScene.upAccentInstance.reset();
        m_oScene.upGroundInstance.reset();
        m_oScene.upCubeInstance.reset();
        m_oScene.oCubeMesh.Reset();
        LogShaderCacheStatistics();
        m_oScene.upMaterial.reset();
        m_oScene.upNormalMap.reset();
        m_oScene.upCheckerboard.reset();
        m_oScene.upGroundIndexBuffer.reset();
        m_oScene.upGroundVertexBuffer.reset();
        Flawy::Core::Logger::Get().info("Sandbox shut down");
    }

    void SandboxApplication::LogShaderCacheStatistics()
    {
        const Flawy::Renderer::ShaderCache& oCache = GetMaterialSystem().GetShaderCache();
        if (!oCache.IsEnabled())
        {
            Flawy::Core::Logger::Get().info("Shader cache disabled (use --shader-cache=<dir>)");
            return;
        }

        const Flawy::Renderer::ShaderCacheStatistics& oStatistics = oCache.GetStatistics();
        Flawy::Core::Logger::Get().info("Shader cache: {} hit(s), {} miss(es), {} store(s); material permutations: {}; library programs: {}",
                                        oStatistics.uDiskHits,
                                        oStatistics.uMisses,
                                        oStatistics.uStores,
                                        m_oScene.upMaterial ? m_oScene.upMaterial->GetPermutationCount() : 0u,
                                        GetShaderLibrary().GetProgramCount());
    }

    uint32_t SandboxApplication::GetScreenshotFrame() const noexcept
    {
        if (m_oOptions.uFrameLimit == 0)
        {
            return kDefaultScreenshotFrame;
        }
        return m_oOptions.uFrameLimit > kScreenshotLeadFrames ? m_oOptions.uFrameLimit - kScreenshotLeadFrames : 1;
    }
}
