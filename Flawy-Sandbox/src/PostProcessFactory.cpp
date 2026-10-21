#include "PostProcessFactory.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <format>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <string_view>

namespace Sandbox
{
    namespace
    {
        constexpr std::string_view kPostProcessShaderPath = "PostProcess.slang";

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
    }

    void PostProcessFactory::Create(PostProcessResources& oResources, Flawy::Renderer::ShaderCompiler& oCompiler)
    {
        oResources.upProgram = CreateProgram(oCompiler, kPostProcessShaderPath);

        Flawy::Renderer::VertexLayout oLayout;
        oLayout.Add(Flawy::Renderer::VertexAttribute::Position, 2, Flawy::Renderer::VertexAttributeType::Float)
            .Add(Flawy::Renderer::VertexAttribute::TexCoord0, 2, Flawy::Renderer::VertexAttributeType::Float);
        oResources.upVertexBuffer = std::make_unique<Flawy::Renderer::VertexBuffer>(std::as_bytes(std::span(kScreenQuadVertices)), oLayout);
        oResources.upIndexBuffer = std::make_unique<Flawy::Renderer::IndexBuffer>(std::span<const uint16_t>(kScreenQuadIndices));
        oResources.upParameters = std::make_unique<Flawy::Renderer::ShaderParameters>(*oResources.upProgram);
    }
}
