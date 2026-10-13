#pragma once

#include <Flawy/Flawy.hpp>

#include <memory>

namespace Sandbox
{
    class SandboxApplication final : public Flawy::Core::Application
    {
    public:
        explicit SandboxApplication(Flawy::Renderer::RendererBackend eBackend);

    protected:
        void OnStart() override;
        void OnRender() override;
        void OnShutdown() override;

    private:
        void CreateTriangle();

        std::unique_ptr<Flawy::Renderer::ShaderProgram> m_upProgram;
        std::unique_ptr<Flawy::Renderer::VertexBuffer> m_upVertexBuffer;
    };
}
