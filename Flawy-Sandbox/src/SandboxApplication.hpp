#pragma once

#include "AnimationDriver.hpp"
#include "AnimationResources.hpp"
#include "DepthPrePass.hpp"
#include "ImportResources.hpp"
#include "PostProcessPass.hpp"
#include "PostProcessResources.hpp"
#include "SceneResources.hpp"
#include "ScenePass.hpp"

#include <Flawy/Flawy.hpp>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace Sandbox
{
    struct SandboxOptions
    {
        Flawy::Renderer::RendererBackend eBackend = Flawy::Renderer::RendererBackend::Default;
        uint32_t uFrameLimit = 0;
        std::optional<std::filesystem::path> optScreenshotPath;
        std::optional<std::filesystem::path> optShaderCacheDirectory;
        std::optional<std::filesystem::path> optAssetRoot;
        std::optional<std::string> optImportPath;
    };

    class SandboxApplication final : public Flawy::Core::Application
    {
    public:
        explicit SandboxApplication(SandboxOptions oOptions);

    protected:
        void OnStart() override;
        void OnUpdate(double dDeltaSeconds) override;
        void OnRender(Flawy::Renderer::RenderGraph& oGraph) override;
        void OnShutdown() override;

    private:
        [[nodiscard]] uint32_t GetScreenshotFrame() const noexcept;
        void LogShaderCacheStatistics();

        SandboxOptions m_oOptions;
        uint32_t m_uFrameIndex = 0;
        double m_dElapsedSeconds = 0.0;
        SceneResources m_oScene;
        AnimationResources m_oAnimation;
        AnimationDriver m_oAnimationDriver;
        ImportResources m_oImport;
        DepthPrePass m_oDepthPrePass;
        ScenePass m_oScenePass;
        PostProcessResources m_oPostProcess;
        PostProcessPass m_oPostProcessPass;
        Flawy::Renderer::RenderTargetDescriptor m_oSceneColorDescriptor;
    };
}
