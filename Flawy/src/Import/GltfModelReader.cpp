#include "Import/GltfModelReader.hpp"

#include "Import/GltfAccessorReader.hpp"
#include "Import/GltfAnimationReader.hpp"
#include "Import/GltfMeshBuilder.hpp"
#include "Import/GltfSceneGraph.hpp"
#include "Import/GltfSkinReader.hpp"
#include "Import/PathText.hpp"
#include "Import/SkeletonAssembler.hpp"

#include "Flawy/Assets/IAssetImporter.hpp"

#include <fastgltf/core.hpp>
#include <fastgltf/types.hpp>

#include <glm/mat4x4.hpp>

#include <array>
#include <format>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>

namespace Flawy::Import
{
    namespace
    {
        constexpr std::array<std::string_view, 2> kExtensions = {".gltf", ".glb"};
        constexpr std::string_view kFormatName = "glTF";

        constexpr fastgltf::Extensions kSupportedExtensions =
            fastgltf::Extensions::KHR_texture_transform | fastgltf::Extensions::KHR_texture_basisu | fastgltf::Extensions::MSFT_texture_dds |
            fastgltf::Extensions::KHR_mesh_quantization | fastgltf::Extensions::KHR_lights_punctual | fastgltf::Extensions::EXT_texture_webp |
            fastgltf::Extensions::KHR_materials_specular | fastgltf::Extensions::KHR_materials_ior | fastgltf::Extensions::KHR_materials_iridescence |
            fastgltf::Extensions::KHR_materials_volume | fastgltf::Extensions::KHR_materials_transmission |
            fastgltf::Extensions::KHR_materials_clearcoat | fastgltf::Extensions::KHR_materials_emissive_strength |
            fastgltf::Extensions::KHR_materials_sheen | fastgltf::Extensions::KHR_materials_unlit | fastgltf::Extensions::KHR_materials_anisotropy |
            fastgltf::Extensions::KHR_materials_dispersion | fastgltf::Extensions::KHR_materials_variants;

        constexpr fastgltf::Options kParserOptions =
            fastgltf::Options::LoadExternalBuffers | fastgltf::Options::DecomposeNodeMatrices | fastgltf::Options::GenerateMeshIndices;

        ImportedScene ReadScene(const fastgltf::Asset& oAsset, const GltfAccessorReader& oAccessors, const Assets::AssetImportOptions& oOptions, const std::string& sDefaultName)
        {
            ImportedScene oScene;
            const GltfSceneGraph oGraph(oAsset);
            GltfSkinReader oSkin(oAsset, oGraph, oAccessors, oScene.vWarnings);
            if (oOptions.bImportSkeletons)
            {
                oSkin.Read(sDefaultName);
            }
            if (oSkin.HasSkeleton())
            {
                oScene.optSkeleton = oSkin.GetSkeleton().GetDescriptor();
            }

            if (oOptions.bImportMeshes)
            {
                for (const size_t uNode : oGraph.GetSceneNodes())
                {
                    const fastgltf::Node& oNode = oAsset.nodes[uNode];
                    if (!oNode.meshIndex)
                    {
                        continue;
                    }
                    const SkeletonAssembler* pSkeleton = oSkin.IsBoundToSkeleton(oNode) ? &oSkin.GetSkeleton() : nullptr;
                    const std::optional<glm::mat4> optBakeTransform = oNode.skinIndex ? std::nullopt : std::optional<glm::mat4>(oGraph.GetWorldMatrix(uNode));
                    std::optional<Assets::MeshData> optMesh = GltfMeshBuilder(oAsset, oAccessors, pSkeleton, oScene.vWarnings).Build(uNode, optBakeTransform);
                    if (optMesh)
                    {
                        oScene.vMeshes.push_back(std::move(*optMesh));
                    }
                }
            }

            if (oSkin.HasSkeleton() && oOptions.bImportAnimations)
            {
                GltfAnimationReader oAnimations(oAsset, oAccessors, oSkin, oScene.vWarnings);
                for (size_t uAnimation = 0; uAnimation < oAsset.animations.size(); ++uAnimation)
                {
                    std::optional<Animation::AnimationClipDescriptor> optClip = oAnimations.Read(uAnimation);
                    if (optClip)
                    {
                        oScene.vClips.push_back(std::move(*optClip));
                    }
                }
            }
            return oScene;
        }
    }

    std::string_view GltfModelReader::GetFormatName() const noexcept
    {
        return kFormatName;
    }

    std::span<const std::string_view> GltfModelReader::GetExtensions() const noexcept
    {
        return kExtensions;
    }

    ImportedScene GltfModelReader::Read(const std::filesystem::path& oPath, const Assets::AssetImportOptions& oOptions)
    {
        const std::string sDisplayPath = PathText::ToUtf8(oPath);
        if (!std::filesystem::is_regular_file(oPath))
        {
            throw std::runtime_error(std::format("glTF file '{}' does not exist", sDisplayPath));
        }
        fastgltf::Expected<fastgltf::GltfDataBuffer> oData = fastgltf::GltfDataBuffer::FromPath(oPath);
        if (oData.error() != fastgltf::Error::None)
        {
            throw std::runtime_error(std::format("glTF file '{}' could not be read: {}", sDisplayPath, fastgltf::getErrorMessage(oData.error())));
        }

        fastgltf::Parser oParser(kSupportedExtensions);
        fastgltf::Expected<fastgltf::Asset> oAsset = oParser.loadGltf(oData.get(), oPath.parent_path(), kParserOptions);
        if (oAsset.error() != fastgltf::Error::None)
        {
            throw std::runtime_error(std::format("glTF file '{}' could not be parsed: {}", sDisplayPath, fastgltf::getErrorMessage(oAsset.error())));
        }

        try
        {
            const GltfAccessorReader oAccessors(oAsset.get());
            oAccessors.ValidateStorage();
            const fastgltf::Error eValidation = fastgltf::validate(oAsset.get());
            if (eValidation != fastgltf::Error::None)
            {
                throw std::runtime_error(std::string(fastgltf::getErrorMessage(eValidation)));
            }
            return ReadScene(oAsset.get(), oAccessors, oOptions, PathText::GetStem(oPath));
        }
        catch (const std::runtime_error& oError)
        {
            throw std::runtime_error(std::format("glTF file '{}' is invalid: {}", sDisplayPath, oError.what()));
        }
    }
}
