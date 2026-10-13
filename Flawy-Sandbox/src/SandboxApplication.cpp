#include "SandboxApplication.hpp"

#include <glm/gtc/matrix_transform.hpp>

#include <array>
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace Sandbox
{
    namespace
    {
        constexpr std::string_view kUnlitShaderPath = "Unlit.slang";
        constexpr Flawy::Renderer::ViewId kMainView = 0;
        constexpr float kTriangleOffsetX = 0.25f;

        struct ColoredVertex
        {
            float fX;
            float fY;
            float fZ;
            uint32_t uAbgr;
        };

        constexpr std::array<ColoredVertex, 3> kTriangleVertices = {{
            {0.0f, 0.5f, 0.0f, 0xff0000ff},
            {0.5f, -0.5f, 0.0f, 0xff00ff00},
            {-0.5f, -0.5f, 0.0f, 0xffff0000},
        }};

        Flawy::Core::ApplicationSpecification BuildSpecification(Flawy::Renderer::RendererBackend eBackend)
        {
            Flawy::Core::ApplicationSpecification oSpecification;
            oSpecification.sName = "Flawy-Sandbox";
            oSpecification.oWindow = {.sTitle = "Flawy-Sandbox", .uWidth = 1280, .uHeight = 720, .bResizable = true};
            oSpecification.eRendererBackend = eBackend;
            return oSpecification;
        }

        std::string_view ToString(Flawy::Renderer::ShaderStage eStage) noexcept
        {
            switch (eStage)
            {
            case Flawy::Renderer::ShaderStage::Vertex:
                return "Vertex";
            case Flawy::Renderer::ShaderStage::Fragment:
                return "Fragment";
            case Flawy::Renderer::ShaderStage::Compute:
                return "Compute";
            }
            return "Unknown";
        }

        std::string_view ToString(Flawy::Renderer::ShaderUniformType eType) noexcept
        {
            switch (eType)
            {
            case Flawy::Renderer::ShaderUniformType::Vec4:
                return "Vec4";
            case Flawy::Renderer::ShaderUniformType::Mat3:
                return "Mat3";
            case Flawy::Renderer::ShaderUniformType::Mat4:
                return "Mat4";
            case Flawy::Renderer::ShaderUniformType::Sampler:
                return "Sampler";
            }
            return "Unknown";
        }

        std::string_view ToString(Flawy::Renderer::VertexAttribute eAttribute) noexcept
        {
            constexpr std::array<std::string_view, 18> kNames = {
                "Position", "Normal", "Tangent", "Bitangent", "Color0", "Color1", "Color2", "Color3", "Indices",
                "Weight", "TexCoord0", "TexCoord1", "TexCoord2", "TexCoord3", "TexCoord4", "TexCoord5", "TexCoord6", "TexCoord7",
            };
            return kNames[static_cast<size_t>(eAttribute)];
        }

        void LogCompiledShader(const Flawy::Renderer::CompiledShader& oShader)
        {
            auto& oLogger = Flawy::Core::Logger::Get();
            const Flawy::Renderer::ShaderInterface& oInterface = oShader.oInterface;
            oLogger.info("Compiled {} shader {}::{} ({} bytes, uniform buffer {} bytes)",
                         ToString(oShader.eStage),
                         oShader.sName,
                         oShader.sEntryPoint,
                         oShader.vBgfxBinary.size(),
                         oInterface.uUniformBufferSize);

            for (const Flawy::Renderer::ShaderUniform& oUniform : oInterface.vUniforms)
            {
                oLogger.info("  uniform {} ({}, count {}{})", oUniform.sName, ToString(oUniform.eType), oUniform.uCount, oUniform.bPredefined ? ", predefined" : "");
            }

            std::string sAttributes;
            for (const Flawy::Renderer::VertexAttribute eAttribute : oInterface.vVertexAttributes)
            {
                sAttributes.append(sAttributes.empty() ? "" : ", ");
                sAttributes.append(ToString(eAttribute));
            }
            oLogger.info("  attributes: {}", sAttributes.empty() ? "none" : sAttributes);
        }
    }

    SandboxApplication::SandboxApplication(Flawy::Renderer::RendererBackend eBackend)
        : Application(BuildSpecification(eBackend))
    {
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

        CreateTriangle();
    }

    void SandboxApplication::OnRender()
    {
        auto& oRenderDevice = GetRenderDevice();
        oRenderDevice.SetViewTransform(kMainView, glm::mat4(1.0f), glm::mat4(1.0f));
        oRenderDevice.Submit(kMainView, {m_upVertexBuffer.get(), m_upProgram.get(), glm::translate(glm::mat4(1.0f), glm::vec3(kTriangleOffsetX, 0.0f, 0.0f))});
    }

    void SandboxApplication::OnShutdown()
    {
        m_upVertexBuffer.reset();
        m_upProgram.reset();
        Flawy::Core::Logger::Get().info("Sandbox shut down");
    }

    void SandboxApplication::CreateTriangle()
    {
        const Flawy::Renderer::EmbeddedShaderSourceProvider oProvider;
        const std::optional<Flawy::Renderer::ShaderSource> optSource = oProvider.Load(kUnlitShaderPath);
        if (!optSource)
        {
            throw std::runtime_error("Embedded shader not found: " + std::string(kUnlitShaderPath));
        }

        Flawy::Renderer::ShaderCompiler oCompiler(GetRenderDevice().GetBackend());
        const Flawy::Renderer::CompiledShader oVertexShader = oCompiler.Compile(*optSource, Flawy::Renderer::ShaderStage::Vertex, "VertexMain");
        const Flawy::Renderer::CompiledShader oFragmentShader = oCompiler.Compile(*optSource, Flawy::Renderer::ShaderStage::Fragment, "FragmentMain");
        LogCompiledShader(oVertexShader);
        LogCompiledShader(oFragmentShader);

        m_upProgram = std::make_unique<Flawy::Renderer::ShaderProgram>(oVertexShader, oFragmentShader);

        Flawy::Renderer::VertexLayout oLayout;
        oLayout.Add(Flawy::Renderer::VertexAttribute::Position, 3, Flawy::Renderer::VertexAttributeType::Float)
            .Add(Flawy::Renderer::VertexAttribute::Color0, 4, Flawy::Renderer::VertexAttributeType::Uint8, true);
        m_upVertexBuffer = std::make_unique<Flawy::Renderer::VertexBuffer>(std::as_bytes(std::span(kTriangleVertices)), oLayout);
    }
}
