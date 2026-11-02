#include "Flawy/Import/SampleModelWriter.hpp"

#include "Import/PathText.hpp"

#include "Flawy/Core/FileBytes.hpp"

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>

#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <format>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Flawy::Import
{
    namespace
    {
        constexpr uint32_t kSegments = 8;
        constexpr uint32_t kRings = 3;
        constexpr float kRadius = 0.15f;
        constexpr float kRingSpacing = 0.5f;
        constexpr float kBendDegrees = 50.0f;
        constexpr std::array<float, 5> kKeyTimes = {0.0f, 0.5f, 1.0f, 1.5f, 2.0f};
        constexpr std::array<float, 5> kKeyAngles = {0.0f, 1.0f, 0.0f, -1.0f, 0.0f};
        constexpr std::array<std::array<float, 2>, kRings> kRingWeights = {{{1.0f, 0.0f}, {0.5f, 0.5f}, {0.0f, 1.0f}}};
        constexpr std::string_view kRootJointName = "Root";
        constexpr std::string_view kBendJointName = "Bend";
        constexpr std::string_view kMaterialName = "TubeMaterial";
        constexpr glm::vec3 kBendAxis = {0.0f, 0.0f, 1.0f};

        struct TubeGeometry
        {
            std::vector<glm::vec3> vPositions;
            std::vector<glm::vec3> vNormals;
            std::vector<glm::vec2> vTexCoords;
            std::vector<glm::u8vec4> vJoints;
            std::vector<glm::vec4> vWeights;
            std::vector<uint16_t> vIndices;
        };

        TubeGeometry BuildTubeGeometry()
        {
            TubeGeometry oGeometry;
            for (uint32_t uRing = 0; uRing < kRings; ++uRing)
            {
                const float fHeight = static_cast<float>(uRing) * kRingSpacing;
                for (uint32_t uSegment = 0; uSegment < kSegments; ++uSegment)
                {
                    const float fAngle = glm::two_pi<float>() * static_cast<float>(uSegment) / static_cast<float>(kSegments);
                    const glm::vec3 vecDirection(std::cos(fAngle), 0.0f, std::sin(fAngle));
                    oGeometry.vPositions.push_back(vecDirection * kRadius + glm::vec3(0.0f, fHeight, 0.0f));
                    oGeometry.vNormals.push_back(vecDirection);
                    oGeometry.vTexCoords.emplace_back(static_cast<float>(uSegment) / static_cast<float>(kSegments),
                                                      1.0f - static_cast<float>(uRing) / static_cast<float>(kRings - 1));
                    oGeometry.vJoints.emplace_back(0, 1, 0, 0);
                    oGeometry.vWeights.emplace_back(kRingWeights[uRing][0], kRingWeights[uRing][1], 0.0f, 0.0f);
                }
            }

            for (uint32_t uRing = 0; uRing + 1 < kRings; ++uRing)
            {
                for (uint32_t uSegment = 0; uSegment < kSegments; ++uSegment)
                {
                    const auto uA = static_cast<uint16_t>(uRing * kSegments + uSegment);
                    const auto uB = static_cast<uint16_t>(uRing * kSegments + (uSegment + 1) % kSegments);
                    const auto uC = static_cast<uint16_t>((uRing + 1) * kSegments + uSegment);
                    const auto uD = static_cast<uint16_t>((uRing + 1) * kSegments + (uSegment + 1) % kSegments);
                    oGeometry.vIndices.insert(oGeometry.vIndices.end(), {uA, uC, uB, uB, uC, uD});
                }
            }
            return oGeometry;
        }

        class GlbBuilder final
        {
        public:
            GlbBuilder()
            {
                fastgltf::AssetInfo oInfo;
                oInfo.gltfVersion = "2.0";
                oInfo.generator = "Flawy SampleModelWriter";
                m_oAsset.assetInfo = std::move(oInfo);
            }

            template <typename TElement>
            size_t AddAccessor(std::span<const TElement> spanValues,
                               fastgltf::AccessorType eType,
                               fastgltf::ComponentType eComponent,
                               std::optional<fastgltf::BufferTarget> optTarget,
                               std::span<const double> spanMinimum = {},
                               std::span<const double> spanMaximum = {})
            {
                const size_t uOffset = m_vBytes.size();
                const size_t uLength = spanValues.size_bytes();
                m_vBytes.resize(uOffset + uLength);
                std::memcpy(m_vBytes.data() + uOffset, spanValues.data(), uLength);

                fastgltf::BufferView oView;
                oView.bufferIndex = 0;
                oView.byteOffset = uOffset;
                oView.byteLength = uLength;
                if (optTarget)
                {
                    oView.target = *optTarget;
                }
                m_oAsset.bufferViews.push_back(std::move(oView));

                fastgltf::Accessor oAccessor;
                oAccessor.byteOffset = 0;
                oAccessor.count = spanValues.size();
                oAccessor.type = eType;
                oAccessor.componentType = eComponent;
                oAccessor.bufferViewIndex = m_oAsset.bufferViews.size() - 1;
                if (!spanMinimum.empty())
                {
                    oAccessor.min = MakeBounds(spanMinimum);
                    oAccessor.max = MakeBounds(spanMaximum);
                }
                m_oAsset.accessors.push_back(std::move(oAccessor));
                return m_oAsset.accessors.size() - 1;
            }

            [[nodiscard]] fastgltf::Asset& GetAsset() noexcept
            {
                return m_oAsset;
            }

            [[nodiscard]] fastgltf::Asset& Finish()
            {
                fastgltf::Buffer oBuffer;
                oBuffer.byteLength = m_vBytes.size();
                oBuffer.data = fastgltf::sources::Vector{std::move(m_vBytes), fastgltf::MimeType::GltfBuffer};
                m_oAsset.buffers.push_back(std::move(oBuffer));
                return m_oAsset;
            }

        private:
            [[nodiscard]] static fastgltf::AccessorBoundsArray MakeBounds(std::span<const double> spanValues)
            {
                fastgltf::AccessorBoundsArray oBounds = fastgltf::AccessorBoundsArray::ForType<double>(spanValues.size());
                for (size_t uComponent = 0; uComponent < spanValues.size(); ++uComponent)
                {
                    oBounds.set<double>(uComponent, spanValues[uComponent]);
                }
                return oBounds;
            }

            fastgltf::Asset m_oAsset;
            std::vector<std::byte> m_vBytes;
        };

        fastgltf::Node MakeJointNode(std::string_view svName, const glm::vec3& vecTranslation)
        {
            fastgltf::Node oNode;
            oNode.name = svName;
            fastgltf::TRS oTransform;
            oTransform.translation = fastgltf::math::fvec3(vecTranslation.x, vecTranslation.y, vecTranslation.z);
            oNode.transform = oTransform;
            return oNode;
        }

        void AddMesh(GlbBuilder& oBuilder, const TubeGeometry& oGeometry)
        {
            const std::array<double, 3> aMinimum = {-kRadius, 0.0, -kRadius};
            const std::array<double, 3> aMaximum = {kRadius, kRingSpacing * static_cast<double>(kRings - 1), kRadius};
            const size_t uPositions = oBuilder.AddAccessor(std::span<const glm::vec3>(oGeometry.vPositions),
                                                           fastgltf::AccessorType::Vec3,
                                                           fastgltf::ComponentType::Float,
                                                           fastgltf::BufferTarget::ArrayBuffer,
                                                           aMinimum,
                                                           aMaximum);
            const size_t uNormals = oBuilder.AddAccessor(std::span<const glm::vec3>(oGeometry.vNormals),
                                                         fastgltf::AccessorType::Vec3,
                                                         fastgltf::ComponentType::Float,
                                                         fastgltf::BufferTarget::ArrayBuffer);
            const size_t uTexCoords = oBuilder.AddAccessor(std::span<const glm::vec2>(oGeometry.vTexCoords),
                                                           fastgltf::AccessorType::Vec2,
                                                           fastgltf::ComponentType::Float,
                                                           fastgltf::BufferTarget::ArrayBuffer);
            const size_t uJoints = oBuilder.AddAccessor(std::span<const glm::u8vec4>(oGeometry.vJoints),
                                                        fastgltf::AccessorType::Vec4,
                                                        fastgltf::ComponentType::UnsignedByte,
                                                        fastgltf::BufferTarget::ArrayBuffer);
            const size_t uWeights = oBuilder.AddAccessor(std::span<const glm::vec4>(oGeometry.vWeights),
                                                         fastgltf::AccessorType::Vec4,
                                                         fastgltf::ComponentType::Float,
                                                         fastgltf::BufferTarget::ArrayBuffer);
            const size_t uIndices = oBuilder.AddAccessor(std::span<const uint16_t>(oGeometry.vIndices),
                                                         fastgltf::AccessorType::Scalar,
                                                         fastgltf::ComponentType::UnsignedShort,
                                                         fastgltf::BufferTarget::ElementArrayBuffer);

            fastgltf::Primitive oPrimitive;
            oPrimitive.type = fastgltf::PrimitiveType::Triangles;
            oPrimitive.attributes.emplace_back(fastgltf::Attribute{"POSITION", uPositions});
            oPrimitive.attributes.emplace_back(fastgltf::Attribute{"NORMAL", uNormals});
            oPrimitive.attributes.emplace_back(fastgltf::Attribute{"TEXCOORD_0", uTexCoords});
            oPrimitive.attributes.emplace_back(fastgltf::Attribute{"JOINTS_0", uJoints});
            oPrimitive.attributes.emplace_back(fastgltf::Attribute{"WEIGHTS_0", uWeights});
            oPrimitive.indicesAccessor = uIndices;
            oPrimitive.materialIndex = 0;

            fastgltf::Mesh oMesh;
            oMesh.name = SampleModelWriter::kTubeMeshName;
            oMesh.primitives.push_back(std::move(oPrimitive));

            fastgltf::Asset& oAsset = oBuilder.GetAsset();
            fastgltf::Material oMaterial;
            oMaterial.name = kMaterialName;
            oAsset.materials.push_back(std::move(oMaterial));
            oAsset.meshes.push_back(std::move(oMesh));
        }

        void AddSkin(GlbBuilder& oBuilder)
        {
            std::array<glm::mat4, 2> aInverseBinds = {glm::mat4(1.0f), glm::mat4(1.0f)};
            aInverseBinds[1][3] = glm::vec4(0.0f, -kRingSpacing, 0.0f, 1.0f);
            const size_t uInverseBinds =
                oBuilder.AddAccessor(std::span<const glm::mat4>(aInverseBinds), fastgltf::AccessorType::Mat4, fastgltf::ComponentType::Float, std::nullopt);

            fastgltf::Skin oSkin;
            oSkin.name = SampleModelWriter::kTubeSkeletonName;
            oSkin.inverseBindMatrices = uInverseBinds;
            oSkin.joints.push_back(1);
            oSkin.joints.push_back(2);
            oBuilder.GetAsset().skins.push_back(std::move(oSkin));
        }

        void AddAnimation(GlbBuilder& oBuilder)
        {
            std::vector<glm::vec4> vRotations;
            for (const float fAngle : kKeyAngles)
            {
                const glm::quat quatRotation = glm::angleAxis(glm::radians(kBendDegrees * fAngle), kBendAxis);
                vRotations.emplace_back(quatRotation.x, quatRotation.y, quatRotation.z, quatRotation.w);
            }

            const std::array<double, 1> aMinimum = {kKeyTimes.front()};
            const std::array<double, 1> aMaximum = {kKeyTimes.back()};
            const size_t uTimes = oBuilder.AddAccessor(std::span<const float>(kKeyTimes),
                                                       fastgltf::AccessorType::Scalar,
                                                       fastgltf::ComponentType::Float,
                                                       std::nullopt,
                                                       aMinimum,
                                                       aMaximum);
            const size_t uValues = oBuilder.AddAccessor(std::span<const glm::vec4>(vRotations), fastgltf::AccessorType::Vec4, fastgltf::ComponentType::Float, std::nullopt);

            fastgltf::Animation oAnimation;
            oAnimation.name = SampleModelWriter::kTubeClipName;
            oAnimation.samplers.push_back({uTimes, uValues, fastgltf::AnimationInterpolation::Linear});
            oAnimation.channels.push_back({0, 2, fastgltf::AnimationPath::Rotation});
            oBuilder.GetAsset().animations.push_back(std::move(oAnimation));
        }

        void AddNodes(GlbBuilder& oBuilder)
        {
            fastgltf::Asset& oAsset = oBuilder.GetAsset();

            fastgltf::Node oMeshNode;
            oMeshNode.name = SampleModelWriter::kTubeMeshName;
            oMeshNode.meshIndex = 0;
            oMeshNode.skinIndex = 0;
            oAsset.nodes.push_back(std::move(oMeshNode));

            fastgltf::Node oRoot = MakeJointNode(kRootJointName, glm::vec3(0.0f));
            oRoot.children.push_back(2);
            oAsset.nodes.push_back(std::move(oRoot));
            oAsset.nodes.push_back(MakeJointNode(kBendJointName, glm::vec3(0.0f, kRingSpacing, 0.0f)));

            fastgltf::Scene oScene;
            oScene.name = "Scene";
            oScene.nodeIndices.push_back(0);
            oScene.nodeIndices.push_back(1);
            oAsset.scenes.push_back(std::move(oScene));
            oAsset.defaultScene = 0;
        }
    }

    void SampleModelWriter::WriteSkinnedTubeGlb(const std::filesystem::path& oPath)
    {
        const TubeGeometry oGeometry = BuildTubeGeometry();
        GlbBuilder oBuilder;
        AddMesh(oBuilder, oGeometry);
        AddSkin(oBuilder);
        AddAnimation(oBuilder);
        AddNodes(oBuilder);
        const fastgltf::Asset& oAsset = oBuilder.Finish();

        fastgltf::Exporter oExporter;
        fastgltf::Expected<fastgltf::ExportResult<std::vector<std::byte>>> oResult = oExporter.writeGltfBinary(oAsset, fastgltf::ExportOptions::ValidateAsset);
        if (oResult.error() != fastgltf::Error::None)
        {
            throw std::runtime_error(std::format("Sample model '{}' could not be exported: {}", PathText::ToUtf8(oPath), fastgltf::getErrorMessage(oResult.error())));
        }
        Core::FileBytes::WriteAtomically(oPath, oResult.get().output);
    }
}
