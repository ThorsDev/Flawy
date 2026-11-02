#pragma once

#include "Import/AssetNameRegistry.hpp"

#include "Flawy/Animation/AnimationClipAsset.hpp"
#include "Flawy/Animation/AnimationClipDescriptor.hpp"
#include "Flawy/Animation/SkeletonAsset.hpp"
#include "Flawy/Assets/AssetHandle.hpp"
#include "Flawy/Assets/IAssetImporter.hpp"
#include "Flawy/Assets/StaticMeshAsset.hpp"
#include "Flawy/Import/ImportedScene.hpp"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Flawy::Import
{
    class ImportedSceneCommitter final
    {
    public:
        explicit ImportedSceneCommitter(const Assets::AssetImportContext& oContext);

        [[nodiscard]] Assets::AssetImportResult Commit(ImportedScene oScene);

    private:
        struct PendingSkeleton
        {
            std::string sPath;
            std::shared_ptr<Animation::SkeletonAsset> spAsset;
            Animation::SkeletonDescriptor oDescriptor;
        };

        struct PendingMesh
        {
            std::string sPath;
            std::shared_ptr<Assets::StaticMeshAsset> spAsset;
        };

        struct PendingClip
        {
            std::string sPath;
            std::shared_ptr<Animation::AnimationClipAsset> spAsset;
            Animation::AnimationClipDescriptor oDescriptor;
        };

        void ApplyOptions(ImportedScene& oScene) const;
        void ReserveNames(ImportedScene& oScene);
        void BuildSkeleton(Animation::SkeletonDescriptor oDescriptor);
        void BuildMesh(Assets::MeshData oMesh, const Animation::SkeletonDescriptor* pDescriptor);
        void BuildClip(Animation::AnimationClipDescriptor oClip);
        void RemapJointIndices(Assets::MeshData& oMesh, const Animation::SkeletonDescriptor* pDescriptor) const;
        void SaveAll(Assets::AssetImportResult& oResult) const;
        void InsertAll(Assets::AssetImportResult& oResult) const;
        void LogSummary(const Assets::AssetImportResult& oResult) const;
        [[nodiscard]] std::string MakeAssetPath(std::string_view svName, std::string_view svExtension) const;
        void ReleaseExisting(const std::string& sPath) const;

        const Assets::AssetImportContext& m_oContext;
        std::optional<std::string> m_optOutputDirectory;
        AssetNameRegistry m_oNames;
        std::optional<PendingSkeleton> m_optSkeleton;
        std::vector<PendingMesh> m_vMeshes;
        std::vector<PendingClip> m_vClips;
    };
}
