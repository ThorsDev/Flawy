#pragma once

#include "PostProcessPass.hpp"
#include "PostProcessResources.hpp"
#include "SceneResources.hpp"
#include "ScenePass.hpp"

#include <Flawy/Flawy.hpp>

#include <cstdint>
#include <filesystem>
#include <optional>

namespace Sandbox
{
    struct SandboxOptions
    {
        Flawy::Renderer::RendererBackend eBackend = Flawy::Renderer::RendererBackend::Default;
        uint32_t uFrameLimit = 0;
        std::optional<std::filesystem::path> optScreenshotPath;
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
        void CreateCube(Flawy::Renderer::ShaderCompiler& oCompiler);
        void CreatePostProcess(Flawy::Renderer::ShaderCompiler& oCompiler);
        [[nodiscard]] uint32_t GetScreenshotFrame() const noexcept;

        SandboxOptions m_oOptions;
        uint32_t m_uFrameIndex = 0;
        double m_dElapsedSeconds = 0.0;
        SceneResources m_oScene;
        ScenePass m_oScenePass;
        PostProcessResources m_oPostProcess;
        PostProcessPass m_oPostProcessPass;
        Flawy::Renderer::RenderTargetDescriptor m_oSceneColorDescriptor;
    };
}
