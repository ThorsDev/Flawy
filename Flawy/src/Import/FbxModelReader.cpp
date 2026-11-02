#include "Import/FbxModelReader.hpp"

#include "Import/FbxAnimationReader.hpp"
#include "Import/FbxMeshBuilder.hpp"
#include "Import/FbxSkinBinding.hpp"
#include "Import/PathText.hpp"
#include "Import/UfbxConversion.hpp"

#include "Flawy/Assets/IAssetImporter.hpp"
#include "Flawy/Core/FileBytes.hpp"

#include <ufbx.h>

#include <array>
#include <cstddef>
#include <format>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace Flawy::Import
{
    namespace
    {
        constexpr std::array<std::string_view, 1> kExtensions = {".fbx"};
        constexpr std::string_view kFormatName = "FBX";

        struct UfbxSceneDeleter
        {
            void operator()(ufbx_scene* pScene) const noexcept
            {
                ufbx_free_scene(pScene);
            }
        };

        ImportedScene ReadScene(const ufbx_scene& oSource, const Assets::AssetImportOptions& oOptions, const std::string& sDefaultName)
        {
            ImportedScene oScene;
            FbxSkinBinding oSkinBinding(oSource, oScene.vWarnings);
            if (oOptions.bImportSkeletons)
            {
                oSkinBinding.Build(sDefaultName);
            }
            if (oSkinBinding.HasSkeleton())
            {
                oScene.optSkeleton = oSkinBinding.GetSkeleton().GetDescriptor();
            }

            if (oOptions.bImportMeshes)
            {
                for (size_t uNode = 0; uNode < oSource.nodes.count; ++uNode)
                {
                    const ufbx_node* pNode = oSource.nodes.data[uNode];
                    if (pNode->mesh == nullptr)
                    {
                        continue;
                    }
                    std::optional<Assets::MeshData> optMesh = FbxMeshBuilder(oSkinBinding, oScene.vWarnings).Build(*pNode);
                    if (optMesh)
                    {
                        oScene.vMeshes.push_back(std::move(*optMesh));
                    }
                }
            }

            if (oSkinBinding.HasSkeleton() && oOptions.bImportAnimations)
            {
                FbxAnimationReader oAnimations(oSource, oSkinBinding, oScene.vWarnings);
                for (size_t uStack = 0; uStack < oSource.anim_stacks.count; ++uStack)
                {
                    std::optional<Animation::AnimationClipDescriptor> optClip = oAnimations.Read(*oSource.anim_stacks.data[uStack], uStack);
                    if (optClip)
                    {
                        oScene.vClips.push_back(std::move(*optClip));
                    }
                }
            }
            return oScene;
        }
    }

    std::string_view FbxModelReader::GetFormatName() const noexcept
    {
        return kFormatName;
    }

    std::span<const std::string_view> FbxModelReader::GetExtensions() const noexcept
    {
        return kExtensions;
    }

    ImportedScene FbxModelReader::Read(const std::filesystem::path& oPath, const Assets::AssetImportOptions& oOptions)
    {
        const std::string sDisplayPath = PathText::ToUtf8(oPath);
        const std::vector<std::byte> vBytes = Core::FileBytes::Read(oPath);

        ufbx_load_opts oLoadOptions{};
        oLoadOptions.target_axes = ufbx_axes_right_handed_y_up;
        oLoadOptions.target_unit_meters = 1.0;
        oLoadOptions.space_conversion = UFBX_SPACE_CONVERSION_TRANSFORM_ROOT;
        oLoadOptions.generate_missing_normals = true;
        oLoadOptions.ignore_animation = !oOptions.bImportAnimations;

        ufbx_error oError{};
        const std::unique_ptr<ufbx_scene, UfbxSceneDeleter> upScene(ufbx_load_memory(vBytes.data(), vBytes.size(), &oLoadOptions, &oError));
        if (!upScene)
        {
            throw std::runtime_error(std::format("FBX file '{}' could not be loaded: {}", sDisplayPath, UfbxConversion::FormatError(oError)));
        }

        try
        {
            return ReadScene(*upScene, oOptions, PathText::GetStem(oPath));
        }
        catch (const std::runtime_error& oFailure)
        {
            throw std::runtime_error(std::format("FBX file '{}' is invalid: {}", sDisplayPath, oFailure.what()));
        }
    }
}
