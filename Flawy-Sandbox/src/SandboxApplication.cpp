#include "SandboxApplication.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace Sandbox
{
    namespace
    {
        constexpr std::string_view kTexturedShaderPath = "Textured.slang";
        constexpr std::string_view kPostProcessShaderPath = "PostProcess.slang";
        constexpr std::string_view kSceneColorName = "SceneColor";
        constexpr float kVignetteStrength = 0.6f;
        constexpr float kTintPulseRadiansPerSecond = 2.0f;
        constexpr float kTintPulseAmplitude = 0.15f;
        constexpr uint32_t kScreenshotLeadFrames = 10;
        constexpr uint32_t kDefaultScreenshotFrame = 60;
        constexpr uint32_t kCheckerboardSize = 4;
        constexpr uint32_t kCheckerLight = 0xffffffff;
        constexpr uint32_t kCheckerDark = 0xff404040;
        constexpr glm::vec4 kTint = {1.0f, 0.9f, 0.75f, 1.0f};
        constexpr float kYawRadiansPerSecond = 0.9f;
        constexpr float kPitchRadiansPerSecond = 0.5f;

        struct ColoredTexturedVertex
        {
            float fX;
            float fY;
            float fZ;
            float fU;
            float fV;
            uint32_t uAbgr;
        };

        struct CubeFace
        {
            glm::vec3 vecNormal;
            glm::vec3 vecRight;
            glm::vec3 vecUp;
            uint32_t uAbgr;
        };

        constexpr std::array<CubeFace, 6> kCubeFaces = {{
            {{0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 0xff4040ff},
            {{0.0f, 0.0f, -1.0f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, 0xff40ff40},
            {{1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, 0xffff4040},
            {{-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f, 0.0f}, 0xff40ffff},
            {{0.0f, 1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, -1.0f}, 0xffff40ff},
            {{0.0f, -1.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f}, 0xffffff40},
        }};

        constexpr std::array<std::pair<float, float>, 4> kFaceCorners = {{{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}}};
        constexpr std::array<uint16_t, 6> kFaceIndices = {0, 2, 1, 0, 3, 2};

        struct ScreenVertex
        {
            float fX;
            float fY;
            float fU;
            float fV;
        };

        constexpr std::array<ScreenVertex, 4> kScreenQuadVertices = {{
            {-1.0f, 1.0f, 0.0f, 0.0f},
            {1.0f, 1.0f, 1.0f, 0.0f},
            {1.0f, -1.0f, 1.0f, 1.0f},
            {-1.0f, -1.0f, 0.0f, 1.0f},
        }};
        constexpr std::array<uint16_t, 6> kScreenQuadIndices = {0, 2, 1, 0, 3, 2};

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

        std::string DescribeUniform(const Flawy::Renderer::ShaderUniform& oUniform)
        {
            std::string sDetails = std::format("{}, count {}", ToString(oUniform.eType), oUniform.uCount);
            if (oUniform.eType == Flawy::Renderer::ShaderUniformType::Sampler)
            {
                sDetails.append(std::format(", stage {}", oUniform.uTextureStage));
            }
            if (oUniform.bPredefined)
            {
                sDetails.append(", predefined");
            }
            return sDetails;
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
                oLogger.info("  uniform {} ({})", oUniform.sName, DescribeUniform(oUniform));
            }

            std::string sAttributes;
            for (const Flawy::Renderer::VertexAttribute eAttribute : oInterface.vVertexAttributes)
            {
                sAttributes.append(sAttributes.empty() ? "" : ", ");
                sAttributes.append(ToString(eAttribute));
            }
            oLogger.info("  attributes: {}", sAttributes.empty() ? "none" : sAttributes);
        }

        std::unique_ptr<Flawy::Renderer::ShaderProgram> CreateProgram(Flawy::Renderer::ShaderCompiler& oCompiler, std::string_view svPath)
        {
            const Flawy::Renderer::EmbeddedShaderSourceProvider oProvider;
            const std::optional<Flawy::Renderer::ShaderSource> optSource = oProvider.Load(svPath);
            if (!optSource)
            {
                throw std::runtime_error("Embedded shader not found: " + std::string(svPath));
            }

            const Flawy::Renderer::CompiledShader oVertexShader = oCompiler.Compile(*optSource, Flawy::Renderer::ShaderStage::Vertex, "VertexMain");
            const Flawy::Renderer::CompiledShader oFragmentShader = oCompiler.Compile(*optSource, Flawy::Renderer::ShaderStage::Fragment, "FragmentMain");
            LogCompiledShader(oVertexShader);
            LogCompiledShader(oFragmentShader);
            return std::make_unique<Flawy::Renderer::ShaderProgram>(oVertexShader, oFragmentShader);
        }

        uint32_t ToCubeVertexIndex(size_t uFace, uint16_t uCorner) noexcept
        {
            return static_cast<uint32_t>(uFace * kFaceCorners.size() + uCorner);
        }

        std::array<ColoredTexturedVertex, kCubeFaces.size() * kFaceCorners.size()> BuildCubeVertices() noexcept
        {
            std::array<ColoredTexturedVertex, kCubeFaces.size() * kFaceCorners.size()> aVertices{};
            for (size_t uFace = 0; uFace < kCubeFaces.size(); ++uFace)
            {
                const CubeFace& oFace = kCubeFaces[uFace];
                for (uint16_t uCorner = 0; uCorner < kFaceCorners.size(); ++uCorner)
                {
                    const auto [fU, fV] = kFaceCorners[uCorner];
                    const glm::vec3 vecPosition = oFace.vecNormal * 0.5f + oFace.vecRight * (fU - 0.5f) + oFace.vecUp * (0.5f - fV);
                    aVertices[ToCubeVertexIndex(uFace, uCorner)] = {vecPosition.x, vecPosition.y, vecPosition.z, fU, fV, oFace.uAbgr};
                }
            }
            return aVertices;
        }

        std::array<uint16_t, kCubeFaces.size() * kFaceIndices.size()> BuildCubeIndices() noexcept
        {
            std::array<uint16_t, kCubeFaces.size() * kFaceIndices.size()> aIndices{};
            for (size_t uFace = 0; uFace < kCubeFaces.size(); ++uFace)
            {
                for (size_t uIndex = 0; uIndex < kFaceIndices.size(); ++uIndex)
                {
                    aIndices[uFace * kFaceIndices.size() + uIndex] = static_cast<uint16_t>(ToCubeVertexIndex(uFace, kFaceIndices[uIndex]));
                }
            }
            return aIndices;
        }

        std::array<uint32_t, kCheckerboardSize * kCheckerboardSize> BuildCheckerboard() noexcept
        {
            std::array<uint32_t, kCheckerboardSize * kCheckerboardSize> aPixels{};
            for (uint32_t uY = 0; uY < kCheckerboardSize; ++uY)
            {
                for (uint32_t uX = 0; uX < kCheckerboardSize; ++uX)
                {
                    aPixels[uY * kCheckerboardSize + uX] = (uX + uY) % 2 == 0 ? kCheckerLight : kCheckerDark;
                }
            }
            return aPixels;
        }
    }

    SandboxApplication::SandboxApplication(SandboxOptions oOptions)
        : Application(BuildSpecification(oOptions.eBackend))
        , m_oOptions(std::move(oOptions))
        , m_oScenePass(m_oScene)
        , m_oPostProcessPass(m_oPostProcess)
    {
        m_oSceneColorDescriptor.sDebugName = std::string(kSceneColorName);
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

        Flawy::Renderer::ShaderCompiler oCompiler(oRenderDevice.GetBackend());
        CreateCube(oCompiler);
        CreatePostProcess(oCompiler);
    }

    void SandboxApplication::OnUpdate(double dDeltaSeconds)
    {
        ++m_uFrameIndex;
        m_dElapsedSeconds += dDeltaSeconds;

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
        const auto fSeconds = static_cast<float>(m_dElapsedSeconds);
        const glm::mat4 matYaw = glm::rotate(glm::mat4(1.0f), fSeconds * kYawRadiansPerSecond, glm::vec3(0.0f, 1.0f, 0.0f));
        m_oScenePass.SetModelMatrix(glm::rotate(matYaw, fSeconds * kPitchRadiansPerSecond, glm::vec3(1.0f, 0.0f, 0.0f)));
        const Flawy::Renderer::RenderGraphResource oSceneColor = oGraph.CreateTransientTarget(m_oSceneColorDescriptor);
        m_oScenePass.SetTarget(oSceneColor);
        oGraph.AddPass(m_oScenePass);

        const float fPulse = kTintPulseAmplitude * std::sin(fSeconds * kTintPulseRadiansPerSecond);
        m_oPostProcessPass.SetTint(glm::vec4(1.0f, 1.0f - kTintPulseAmplitude + fPulse, 1.0f - kTintPulseAmplitude - fPulse, kVignetteStrength));
        m_oPostProcessPass.SetSource(oSceneColor);
        m_oPostProcessPass.SetTarget(oGraph.GetBackbuffer());
        oGraph.AddPass(m_oPostProcessPass);
    }

    void SandboxApplication::OnShutdown()
    {
        m_oPostProcess.upParameters.reset();
        m_oPostProcess.upIndexBuffer.reset();
        m_oPostProcess.upVertexBuffer.reset();
        m_oPostProcess.upProgram.reset();
        m_oScene.upParameters.reset();
        m_oScene.upTexture.reset();
        m_oScene.upIndexBuffer.reset();
        m_oScene.upVertexBuffer.reset();
        m_oScene.upProgram.reset();
        Flawy::Core::Logger::Get().info("Sandbox shut down");
    }

    void SandboxApplication::CreateCube(Flawy::Renderer::ShaderCompiler& oCompiler)
    {
        m_oScene.upProgram = CreateProgram(oCompiler, kTexturedShaderPath);

        Flawy::Renderer::VertexLayout oLayout;
        oLayout.Add(Flawy::Renderer::VertexAttribute::Position, 3, Flawy::Renderer::VertexAttributeType::Float)
            .Add(Flawy::Renderer::VertexAttribute::TexCoord0, 2, Flawy::Renderer::VertexAttributeType::Float)
            .Add(Flawy::Renderer::VertexAttribute::Color0, 4, Flawy::Renderer::VertexAttributeType::Uint8, true);

        const auto aVertices = BuildCubeVertices();
        const auto aIndices = BuildCubeIndices();
        m_oScene.upVertexBuffer = std::make_unique<Flawy::Renderer::VertexBuffer>(std::as_bytes(std::span(aVertices)), oLayout);
        m_oScene.upIndexBuffer = std::make_unique<Flawy::Renderer::IndexBuffer>(std::span<const uint16_t>(aIndices));

        Flawy::Renderer::TextureDescriptor oTextureDescriptor;
        oTextureDescriptor.uWidth = kCheckerboardSize;
        oTextureDescriptor.uHeight = kCheckerboardSize;
        oTextureDescriptor.eFormat = Flawy::Renderer::TextureFormat::Rgba8;
        oTextureDescriptor.oSampler.eFilter = Flawy::Renderer::TextureFilter::Point;
        oTextureDescriptor.sDebugName = "Checkerboard";
        const auto aPixels = BuildCheckerboard();
        m_oScene.upTexture = std::make_unique<Flawy::Renderer::Texture>(oTextureDescriptor, std::as_bytes(std::span(aPixels)));

        m_oScene.upParameters = std::make_unique<Flawy::Renderer::ShaderParameters>(*m_oScene.upProgram);
        m_oScene.upParameters->SetTexture(m_oScene.upParameters->Require("s_texColor"), *m_oScene.upTexture);
        m_oScene.upParameters->SetVec4(m_oScene.upParameters->Require("u_tint"), kTint);
    }

    void SandboxApplication::CreatePostProcess(Flawy::Renderer::ShaderCompiler& oCompiler)
    {
        m_oPostProcess.upProgram = CreateProgram(oCompiler, kPostProcessShaderPath);

        Flawy::Renderer::VertexLayout oLayout;
        oLayout.Add(Flawy::Renderer::VertexAttribute::Position, 2, Flawy::Renderer::VertexAttributeType::Float)
            .Add(Flawy::Renderer::VertexAttribute::TexCoord0, 2, Flawy::Renderer::VertexAttributeType::Float);
        m_oPostProcess.upVertexBuffer = std::make_unique<Flawy::Renderer::VertexBuffer>(std::as_bytes(std::span(kScreenQuadVertices)), oLayout);
        m_oPostProcess.upIndexBuffer = std::make_unique<Flawy::Renderer::IndexBuffer>(std::span<const uint16_t>(kScreenQuadIndices));
        m_oPostProcess.upParameters = std::make_unique<Flawy::Renderer::ShaderParameters>(*m_oPostProcess.upProgram);
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
