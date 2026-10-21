#include "SceneFactory.hpp"

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <utility>

namespace Sandbox
{
    namespace
    {
        constexpr std::string_view kGraphName = "SandboxSurface";
        constexpr std::string_view kAlbedoParameter = "Albedo";
        constexpr std::string_view kNormalMapParameter = "NormalMap";
        constexpr std::string_view kOpacityMaskParameter = "OpacityMask";
        constexpr std::string_view kTintParameter = "Tint";
        constexpr std::string_view kMetallicParameter = "Metallic";
        constexpr std::string_view kRoughnessParameter = "Roughness";
        constexpr std::string_view kEmissiveColorParameter = "EmissiveColor";
        constexpr std::string_view kUseNormalMapSwitch = "UseNormalMap";
        constexpr glm::vec4 kFlatTangentNormal = {0.0f, 0.0f, 1.0f, 0.0f};
        constexpr float kFresnelExponent = 3.0f;
        constexpr uint32_t kNormalMapSize = 4;
        constexpr float kNormalTiltStep = 0.18f;
        constexpr uint32_t kCheckerboardSize = 4;
        constexpr uint32_t kCheckerLight = 0xffffffff;
        constexpr uint32_t kCheckerDark = 0xff404040;
        constexpr float kGroundHeight = -0.9f;
        constexpr float kGroundSize = 6.0f;
        constexpr float kGroundTextureRepeat = 3.0f;
        constexpr uint32_t kGroundAbgr = 0xffffffffu;

        struct SurfaceSettings
        {
            glm::vec4 vecTint;
            float fMetallic;
            float fRoughness;
            glm::vec4 vecEmissiveColor;
        };

        constexpr SurfaceSettings kCubeSurface = {{1.0f, 0.9f, 0.75f, 1.0f}, 0.1f, 0.4f, {0.0f, 0.0f, 0.0f, 0.0f}};
        constexpr SurfaceSettings kGroundSurface = {{0.45f, 0.45f, 0.5f, 1.0f}, 0.0f, 0.9f, {0.0f, 0.0f, 0.0f, 0.0f}};
        constexpr SurfaceSettings kAccentSurface = {{1.0f, 0.85f, 0.6f, 1.0f}, 1.0f, 0.3f, {0.6f, 0.25f, 0.05f, 0.0f}};

        struct TangentMeshVertex
        {
            float fX;
            float fY;
            float fZ;
            float fNormalX;
            float fNormalY;
            float fNormalZ;
            float fTangentX;
            float fTangentY;
            float fTangentZ;
            float fTangentW;
            float fU;
            float fV;
            uint32_t uAbgr;
        };

        struct MeshVertex
        {
            float fX;
            float fY;
            float fZ;
            float fNormalX;
            float fNormalY;
            float fNormalZ;
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
        constexpr size_t kTopFaceIndex = 4;

        constexpr std::array<std::pair<float, float>, 4> kFaceCorners = {{{0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}}};
        constexpr std::array<uint16_t, 6> kFaceIndices = {0, 2, 1, 0, 3, 2};

        uint32_t ToCubeVertexIndex(size_t uFace, uint16_t uCorner) noexcept
        {
            return static_cast<uint32_t>(uFace * kFaceCorners.size() + uCorner);
        }

        std::array<TangentMeshVertex, kCubeFaces.size() * kFaceCorners.size()> BuildCubeVertices() noexcept
        {
            std::array<TangentMeshVertex, kCubeFaces.size() * kFaceCorners.size()> aVertices{};
            for (size_t uFace = 0; uFace < kCubeFaces.size(); ++uFace)
            {
                const CubeFace& oFace = kCubeFaces[uFace];
                for (uint16_t uCorner = 0; uCorner < kFaceCorners.size(); ++uCorner)
                {
                    const auto [fU, fV] = kFaceCorners[uCorner];
                    const glm::vec3 vecPosition = oFace.vecNormal * 0.5f + oFace.vecRight * (fU - 0.5f) + oFace.vecUp * (0.5f - fV);
                    aVertices[ToCubeVertexIndex(uFace, uCorner)] = {vecPosition.x, vecPosition.y, vecPosition.z,
                                                                    oFace.vecNormal.x, oFace.vecNormal.y, oFace.vecNormal.z,
                                                                    oFace.vecRight.x, oFace.vecRight.y, oFace.vecRight.z, 1.0f,
                                                                    fU, fV, oFace.uAbgr};
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

        std::array<MeshVertex, kFaceCorners.size()> BuildGroundVertices() noexcept
        {
            const CubeFace& oTop = kCubeFaces[kTopFaceIndex];
            std::array<MeshVertex, kFaceCorners.size()> aVertices{};
            for (uint16_t uCorner = 0; uCorner < kFaceCorners.size(); ++uCorner)
            {
                const auto [fU, fV] = kFaceCorners[uCorner];
                const glm::vec3 vecPosition = glm::vec3(0.0f, kGroundHeight, 0.0f) + (oTop.vecRight * (fU - 0.5f) + oTop.vecUp * (0.5f - fV)) * kGroundSize;
                aVertices[uCorner] = {vecPosition.x, vecPosition.y, vecPosition.z, oTop.vecNormal.x, oTop.vecNormal.y, oTop.vecNormal.z,
                                      fU * kGroundTextureRepeat, fV * kGroundTextureRepeat, kGroundAbgr};
            }
            return aVertices;
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

        std::array<uint32_t, kNormalMapSize * kNormalMapSize> BuildNormalMap() noexcept
        {
            std::array<uint32_t, kNormalMapSize * kNormalMapSize> aPixels{};
            for (uint32_t uY = 0; uY < kNormalMapSize; ++uY)
            {
                for (uint32_t uX = 0; uX < kNormalMapSize; ++uX)
                {
                    const float fTiltX = static_cast<float>(static_cast<int32_t>((uX * 7u + uY * 3u) % 5u) - 2) * kNormalTiltStep;
                    const float fTiltY = static_cast<float>(static_cast<int32_t>((uX * 3u + uY * 5u + 1u) % 5u) - 2) * kNormalTiltStep;
                    const glm::vec3 vecEncoded = glm::normalize(glm::vec3(fTiltX, fTiltY, 1.0f)) * 0.5f + 0.5f;
                    const auto uRed = static_cast<uint32_t>(vecEncoded.x * 255.0f + 0.5f);
                    const auto uGreen = static_cast<uint32_t>(vecEncoded.y * 255.0f + 0.5f);
                    const auto uBlue = static_cast<uint32_t>(vecEncoded.z * 255.0f + 0.5f);
                    aPixels[uY * kNormalMapSize + uX] = 0xff000000u | (uBlue << 16) | (uGreen << 8) | uRed;
                }
            }
            return aPixels;
        }

        Flawy::Renderer::VertexLayout BuildMeshLayout(bool bWithTangent)
        {
            Flawy::Renderer::VertexLayout oLayout;
            oLayout.Add(Flawy::Renderer::VertexAttribute::Position, 3, Flawy::Renderer::VertexAttributeType::Float)
                .Add(Flawy::Renderer::VertexAttribute::Normal, 3, Flawy::Renderer::VertexAttributeType::Float);
            if (bWithTangent)
            {
                oLayout.Add(Flawy::Renderer::VertexAttribute::Tangent, 4, Flawy::Renderer::VertexAttributeType::Float);
            }
            oLayout.Add(Flawy::Renderer::VertexAttribute::TexCoord0, 2, Flawy::Renderer::VertexAttributeType::Float)
                .Add(Flawy::Renderer::VertexAttribute::Color0, 4, Flawy::Renderer::VertexAttributeType::Uint8, true);
            return oLayout;
        }

        std::unique_ptr<Flawy::Renderer::Texture> CreatePixelTexture(std::span<const uint32_t> spanPixels, uint32_t uSize, bool bSrgb, std::string_view svName)
        {
            Flawy::Renderer::TextureDescriptor oDescriptor;
            oDescriptor.uWidth = uSize;
            oDescriptor.uHeight = uSize;
            oDescriptor.eFormat = Flawy::Renderer::TextureFormat::Rgba8;
            oDescriptor.oSampler.eFilter = Flawy::Renderer::TextureFilter::Point;
            oDescriptor.bSrgb = bSrgb;
            oDescriptor.sDebugName = std::string(svName);
            return std::make_unique<Flawy::Renderer::Texture>(oDescriptor, std::as_bytes(spanPixels));
        }

        Flawy::Renderer::MaterialGraph BuildSurfaceGraph()
        {
            using Flawy::Renderer::MaterialNodeId;
            using Flawy::Renderer::MaterialNodeKind;
            using Flawy::Renderer::MaterialOutputSlot;
            using Flawy::Renderer::MaterialValueType;

            Flawy::Renderer::MaterialGraph oGraph;
            oGraph.SetName(kGraphName);
            oGraph.SetShadingModel(Flawy::Renderer::MaterialShadingModel::Lit);

            const MaterialNodeId oAlbedo = oGraph.AddTextureSample(oGraph.AddTextureParameter(kAlbedoParameter, true), MaterialNodeId{});
            const MaterialNodeId oTint = oGraph.AddVectorParameter(kTintParameter, glm::vec4(1.0f), MaterialValueType::Float3);
            const MaterialNodeId oBaseColor = oGraph.AddBinary(MaterialNodeKind::Multiply, oGraph.AddComponentMask(oAlbedo, true, true, true, false), oTint);

            const MaterialNodeId oNormalSample = oGraph.AddTextureSample(oGraph.AddTextureParameter(kNormalMapParameter, false), oGraph.AddTexCoord(0));
            const MaterialNodeId oScaled = oGraph.AddBinary(MaterialNodeKind::Multiply, oGraph.AddComponentMask(oNormalSample, true, true, true, false),
                                                            oGraph.AddConstant(glm::vec4(2.0f), MaterialValueType::Float1));
            const MaterialNodeId oMappedNormal = oGraph.AddBinary(MaterialNodeKind::Subtract, oScaled, oGraph.AddConstant(glm::vec4(1.0f), MaterialValueType::Float1));
            const MaterialNodeId oFlatNormal = oGraph.AddConstant(kFlatTangentNormal, MaterialValueType::Float3);
            const MaterialNodeId oNormal = oGraph.AddStaticSwitch(kUseNormalMapSwitch, oMappedNormal, oFlatNormal, true);

            const MaterialNodeId oFresnel = oGraph.AddFresnel(MaterialNodeId{}, kFresnelExponent);
            const MaterialNodeId oEmissiveColor = oGraph.AddVectorParameter(kEmissiveColorParameter, glm::vec4(0.0f), MaterialValueType::Float3);
            const MaterialNodeId oEmissive = oGraph.AddBinary(MaterialNodeKind::Multiply, oFresnel, oEmissiveColor);

            static_cast<void>(oGraph.AddTextureParameter(kOpacityMaskParameter, false));

            oGraph.ConnectOutput(MaterialOutputSlot::BaseColor, oBaseColor);
            oGraph.ConnectOutput(MaterialOutputSlot::Normal, oNormal);
            oGraph.ConnectOutput(MaterialOutputSlot::Roughness, oGraph.AddScalarParameter(kRoughnessParameter, 0.5f));
            oGraph.ConnectOutput(MaterialOutputSlot::Metallic, oGraph.AddScalarParameter(kMetallicParameter, 0.0f));
            oGraph.ConnectOutput(MaterialOutputSlot::Emissive, oEmissive);
            return oGraph;
        }

        void ApplySurface(Flawy::Renderer::MaterialInstance& oInstance, const SceneResources& oResources, const SurfaceSettings& oSurface)
        {
            oInstance.SetTexture(oInstance.RequireTexture(kAlbedoParameter), *oResources.upCheckerboard);
            oInstance.SetTexture(oInstance.RequireTexture(kNormalMapParameter), *oResources.upNormalMap);
            oInstance.SetVector(oInstance.RequireValue(kTintParameter), oSurface.vecTint);
            oInstance.SetFloat(oInstance.RequireValue(kMetallicParameter), oSurface.fMetallic);
            oInstance.SetFloat(oInstance.RequireValue(kRoughnessParameter), oSurface.fRoughness);
            oInstance.SetVector(oInstance.RequireValue(kEmissiveColorParameter), oSurface.vecEmissiveColor);
        }
    }

    void SceneFactory::Create(SceneResources& oResources, Flawy::Renderer::MaterialSystem& oMaterialSystem)
    {
        CreateMeshes(oResources);
        CreateTextures(oResources);
        CreateMaterial(oResources, oMaterialSystem);
        CreateMaterialInstances(oResources);
    }

    void SceneFactory::CreateMeshes(SceneResources& oResources)
    {
        const auto aCubeVertices = BuildCubeVertices();
        const auto aCubeIndices = BuildCubeIndices();
        oResources.upCubeVertexBuffer = std::make_unique<Flawy::Renderer::VertexBuffer>(std::as_bytes(std::span(aCubeVertices)), BuildMeshLayout(true));
        oResources.upCubeIndexBuffer = std::make_unique<Flawy::Renderer::IndexBuffer>(std::span<const uint16_t>(aCubeIndices));

        const auto aGroundVertices = BuildGroundVertices();
        oResources.upGroundVertexBuffer = std::make_unique<Flawy::Renderer::VertexBuffer>(std::as_bytes(std::span(aGroundVertices)), BuildMeshLayout(false));
        oResources.upGroundIndexBuffer = std::make_unique<Flawy::Renderer::IndexBuffer>(std::span<const uint16_t>(kFaceIndices));
    }

    void SceneFactory::CreateTextures(SceneResources& oResources)
    {
        const auto aCheckerboard = BuildCheckerboard();
        oResources.upCheckerboard = CreatePixelTexture(aCheckerboard, kCheckerboardSize, true, "Checkerboard");
        const auto aNormalMap = BuildNormalMap();
        oResources.upNormalMap = CreatePixelTexture(aNormalMap, kNormalMapSize, false, "NormalMap");
    }

    void SceneFactory::CreateMaterial(SceneResources& oResources, Flawy::Renderer::MaterialSystem& oMaterialSystem)
    {
        auto& oLogger = Flawy::Core::Logger::Get();
        const std::string sJson = BuildSurfaceGraph().ToJson();
        oLogger.info("Material graph '{}' JSON: {}", kGraphName, sJson);
        Flawy::Renderer::MaterialGraph oParsed = Flawy::Renderer::MaterialGraph::FromJson(sJson);
        oResources.upMaterial = std::make_unique<Flawy::Renderer::Material>(oMaterialSystem, std::move(oParsed));
    }

    void SceneFactory::CreateMaterialInstances(SceneResources& oResources)
    {
        Flawy::Renderer::Material& oMaterial = *oResources.upMaterial;
        oResources.upCubeInstance = std::make_unique<Flawy::Renderer::MaterialInstance>(oMaterial);
        oResources.upGroundInstance = std::make_unique<Flawy::Renderer::MaterialInstance>(oMaterial);
        oResources.upAccentInstance = std::make_unique<Flawy::Renderer::MaterialInstance>(oMaterial);
        ApplySurface(*oResources.upCubeInstance, oResources, kCubeSurface);
        ApplySurface(*oResources.upGroundInstance, oResources, kGroundSurface);
        oResources.upGroundInstance->SetStaticSwitch(kUseNormalMapSwitch, false);
        ApplySurface(*oResources.upAccentInstance, oResources, kAccentSurface);
    }
}
