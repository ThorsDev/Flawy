#include "Import/ImportedSceneCommitter.hpp"

#include "Import/ImportLog.hpp"

#include "Flawy/Animation/AnimationClipDescriptorSerializer.hpp"
#include "Flawy/Animation/Skeleton.hpp"
#include "Flawy/Animation/SkeletonDescriptorSerializer.hpp"
#include "Flawy/Assets/AssetId.hpp"
#include "Flawy/Assets/AssetManager.hpp"
#include "Flawy/Assets/MeshDataSerializer.hpp"

#include <cstdint>
#include <format>
#include <limits>
#include <stdexcept>
#include <utility>

namespace Flawy::Import
{
    namespace
    {
        constexpr char kVirtualPathSeparator = '#';
        constexpr std::string_view kSkeletonSuffix = "Skeleton";
        constexpr std::string_view kClipSuffix = "Clip";
        constexpr std::string_view kFallbackDirectoryName = "Import";
        constexpr uint32_t kMaxSkinningJoint = std::numeric_limits<uint8_t>::max();

        std::string GetParentDirectory(std::string_view svPath)
        {
            const size_t uSeparator = svPath.find_last_of('/');
            return uSeparator == std::string_view::npos ? std::string() : std::string(svPath.substr(0, uSeparator));
        }

        std::string JoinPath(std::string_view svDirectory, std::string_view svName)
        {
            return svDirectory.empty() ? std::string(svName) : std::format("{}/{}", svDirectory, svName);
        }

        void DropSkinning(Assets::MeshData& oMesh)
        {
            oMesh.vJointIndices.clear();
            oMesh.vJointWeights.clear();
        }
    }

    ImportedSceneCommitter::ImportedSceneCommitter(const Assets::AssetImportContext& oContext)
        : m_oContext(oContext)
    {
        if (!oContext.oOptions.bSaveToAssetRoot)
        {
            return;
        }

        const std::string sBaseDirectory = oContext.oOptions.sOutputDirectory.empty()
                                               ? GetParentDirectory(oContext.sSourceAssetPath)
                                               : Assets::AssetManager::NormalizePath(oContext.oOptions.sOutputDirectory);
        std::string sDirectoryName = AssetNameRegistry::Sanitize(oContext.sSourceName);
        if (sDirectoryName.empty())
        {
            sDirectoryName = std::string(kFallbackDirectoryName);
        }
        m_optOutputDirectory = Assets::AssetManager::NormalizePath(JoinPath(sBaseDirectory, sDirectoryName));
    }

    Assets::AssetImportResult ImportedSceneCommitter::Commit(ImportedScene oScene)
    {
        ApplyOptions(oScene);
        ReserveNames(oScene);

        const Animation::SkeletonDescriptor* pDescriptor = oScene.optSkeleton ? &*oScene.optSkeleton : nullptr;
        if (pDescriptor != nullptr)
        {
            BuildSkeleton(*pDescriptor);
        }
        for (Assets::MeshData& oMesh : oScene.vMeshes)
        {
            BuildMesh(std::move(oMesh), pDescriptor);
        }
        for (Animation::AnimationClipDescriptor& oClip : oScene.vClips)
        {
            BuildClip(std::move(oClip));
        }

        Assets::AssetImportResult oResult;
        SaveAll(oResult);
        InsertAll(oResult);
        LogSummary(oResult);
        return oResult;
    }

    void ImportedSceneCommitter::ApplyOptions(ImportedScene& oScene) const
    {
        const Assets::AssetImportOptions& oOptions = m_oContext.oOptions;
        if (!oOptions.bImportMeshes)
        {
            oScene.vMeshes.clear();
        }
        if (!oOptions.bImportSkeletons)
        {
            oScene.optSkeleton.reset();
        }
        if (!oOptions.bImportAnimations)
        {
            oScene.vClips.clear();
        }
        if (!oScene.optSkeleton && !oScene.vClips.empty())
        {
            ImportLog::Warn("Import '{}': {} animation clip(s) skipped because no skeleton is imported", m_oContext.sSourceAssetPath, oScene.vClips.size());
            oScene.vClips.clear();
        }
    }

    void ImportedSceneCommitter::ReserveNames(ImportedScene& oScene)
    {
        for (Assets::MeshData& oMesh : oScene.vMeshes)
        {
            oMesh.sName = m_oNames.Reserve(oMesh.sName, m_oContext.sSourceName);
        }
        if (oScene.optSkeleton)
        {
            oScene.optSkeleton->sName = m_oNames.Reserve(oScene.optSkeleton->sName, std::format("{}{}", m_oContext.sSourceName, kSkeletonSuffix));
        }
        for (Animation::AnimationClipDescriptor& oClip : oScene.vClips)
        {
            oClip.sName = m_oNames.Reserve(oClip.sName, std::format("{}{}", m_oContext.sSourceName, kClipSuffix));
        }
    }

    void ImportedSceneCommitter::BuildSkeleton(Animation::SkeletonDescriptor oDescriptor)
    {
        PendingSkeleton oPending;
        oPending.sPath = MakeAssetPath(oDescriptor.sName, Animation::SkeletonAsset::kExtension);
        oPending.spAsset = std::make_shared<Animation::SkeletonAsset>(Assets::AssetId::FromPath(oPending.sPath), oDescriptor);
        oPending.oDescriptor = std::move(oDescriptor);
        m_optSkeleton = std::move(oPending);
    }

    void ImportedSceneCommitter::BuildMesh(Assets::MeshData oMesh, const Animation::SkeletonDescriptor* pDescriptor)
    {
        RemapJointIndices(oMesh, pDescriptor);

        PendingMesh oPending;
        oPending.sPath = MakeAssetPath(oMesh.sName, Assets::StaticMeshAsset::kExtension);
        oPending.spAsset = std::make_shared<Assets::StaticMeshAsset>(Assets::AssetId::FromPath(oPending.sPath), std::move(oMesh));
        m_vMeshes.push_back(std::move(oPending));
    }

    void ImportedSceneCommitter::BuildClip(Animation::AnimationClipDescriptor oClip)
    {
        if (!m_optSkeleton)
        {
            throw std::logic_error(std::format("Import '{}': clip '{}' has no skeleton", m_oContext.sSourceAssetPath, oClip.sName));
        }

        oClip.sSkeletonPath = m_optOutputDirectory ? m_optSkeleton->sPath : std::string();
        PendingClip oPending;
        oPending.sPath = MakeAssetPath(oClip.sName, Animation::AnimationClipAsset::kExtension);
        oPending.spAsset = std::make_shared<Animation::AnimationClipAsset>(Assets::AssetId::FromPath(oPending.sPath),
                                                                          Assets::AssetHandle<Animation::SkeletonAsset>(m_optSkeleton->spAsset),
                                                                          oClip);
        oPending.oDescriptor = std::move(oClip);
        m_vClips.push_back(std::move(oPending));
    }

    void ImportedSceneCommitter::RemapJointIndices(Assets::MeshData& oMesh, const Animation::SkeletonDescriptor* pDescriptor) const
    {
        if (!oMesh.HasSkinning())
        {
            return;
        }
        if (pDescriptor == nullptr || !m_optSkeleton)
        {
            DropSkinning(oMesh);
            return;
        }

        const Animation::Skeleton& oRuntimeSkeleton = m_optSkeleton->spAsset->GetSkeleton();
        std::vector<uint32_t> vRuntimeIndices;
        vRuntimeIndices.reserve(pDescriptor->vJoints.size());
        for (const Animation::JointDescriptor& oJoint : pDescriptor->vJoints)
        {
            const std::optional<uint32_t> optRuntime = oRuntimeSkeleton.FindJoint(oJoint.sName);
            if (!optRuntime)
            {
                throw std::logic_error(std::format("Import '{}': joint '{}' is missing from the built skeleton", m_oContext.sSourceAssetPath, oJoint.sName));
            }
            vRuntimeIndices.push_back(*optRuntime);
        }

        std::vector<glm::u8vec4> vRemapped(oMesh.vJointIndices.size(), glm::u8vec4(0));
        for (size_t uVertex = 0; uVertex < oMesh.vJointIndices.size(); ++uVertex)
        {
            for (glm::length_t iComponent = 0; iComponent < 4; ++iComponent)
            {
                if (oMesh.vJointWeights[uVertex][iComponent] <= 0.0f)
                {
                    continue;
                }
                const uint8_t uSource = oMesh.vJointIndices[uVertex][iComponent];
                if (uSource >= vRuntimeIndices.size())
                {
                    throw std::invalid_argument(std::format("Import '{}': mesh '{}' references joint {} but the skeleton has {} joint(s)",
                                                            m_oContext.sSourceAssetPath,
                                                            oMesh.sName,
                                                            uSource,
                                                            vRuntimeIndices.size()));
                }
                const uint32_t uRuntime = vRuntimeIndices[uSource];
                if (uRuntime > kMaxSkinningJoint)
                {
                    ImportLog::Warn("Import '{}': mesh '{}' is skinned to joint '{}' with runtime index {}, above the {} joints a mesh can reference; skinning data dropped",
                                    m_oContext.sSourceAssetPath,
                                    oMesh.sName,
                                    pDescriptor->vJoints[uSource].sName,
                                    uRuntime,
                                    kMaxSkinningJoint + 1);
                    DropSkinning(oMesh);
                    return;
                }
                vRemapped[uVertex][iComponent] = static_cast<uint8_t>(uRuntime);
            }
        }
        oMesh.vJointIndices = std::move(vRemapped);
    }

    void ImportedSceneCommitter::SaveAll(Assets::AssetImportResult& oResult) const
    {
        if (!m_optOutputDirectory)
        {
            return;
        }

        const Assets::AssetManager& oManager = m_oContext.oManager;
        if (m_optSkeleton)
        {
            Animation::SkeletonDescriptorSerializer::Save(oManager.ResolvePath(m_optSkeleton->sPath), m_optSkeleton->oDescriptor);
            oResult.vSavedPaths.push_back(m_optSkeleton->sPath);
        }
        for (const PendingMesh& oMesh : m_vMeshes)
        {
            Assets::MeshDataSerializer::Save(oManager.ResolvePath(oMesh.sPath), oMesh.spAsset->GetMeshData());
            oResult.vSavedPaths.push_back(oMesh.sPath);
        }
        for (const PendingClip& oClip : m_vClips)
        {
            Animation::AnimationClipDescriptorSerializer::Save(oManager.ResolvePath(oClip.sPath), oClip.oDescriptor);
            oResult.vSavedPaths.push_back(oClip.sPath);
        }
    }

    void ImportedSceneCommitter::InsertAll(Assets::AssetImportResult& oResult) const
    {
        Assets::AssetManager& oManager = m_oContext.oManager;
        if (m_optSkeleton)
        {
            ReleaseExisting(m_optSkeleton->sPath);
            static_cast<void>(oManager.Insert(m_optSkeleton->sPath, m_optSkeleton->spAsset));
            oResult.vAssets.push_back(m_optSkeleton->spAsset);
        }
        for (const PendingMesh& oMesh : m_vMeshes)
        {
            ReleaseExisting(oMesh.sPath);
            static_cast<void>(oManager.Insert(oMesh.sPath, oMesh.spAsset));
            oResult.vAssets.push_back(oMesh.spAsset);
        }
        for (const PendingClip& oClip : m_vClips)
        {
            ReleaseExisting(oClip.sPath);
            static_cast<void>(oManager.Insert(oClip.sPath, oClip.spAsset));
            oResult.vAssets.push_back(oClip.spAsset);
        }
    }

    void ImportedSceneCommitter::LogSummary(const Assets::AssetImportResult& oResult) const
    {
        std::string sSummary = std::format("Imported '{}': {} mesh(es), {} skeleton(s), {} clip(s)",
                                           m_oContext.sSourceAssetPath,
                                           m_vMeshes.size(),
                                           m_optSkeleton ? 1 : 0,
                                           m_vClips.size());
        if (m_optOutputDirectory)
        {
            sSummary.append(std::format("; saved {} asset(s) to '{}'", oResult.vSavedPaths.size(), *m_optOutputDirectory));
        }
        ImportLog::Info("{}", sSummary);
    }

    std::string ImportedSceneCommitter::MakeAssetPath(std::string_view svName, std::string_view svExtension) const
    {
        if (m_optOutputDirectory)
        {
            return Assets::AssetManager::NormalizePath(std::format("{}/{}{}", *m_optOutputDirectory, svName, svExtension));
        }
        return Assets::AssetManager::NormalizePath(std::format("{}{}{}", m_oContext.sSourceAssetPath, kVirtualPathSeparator, svName));
    }

    void ImportedSceneCommitter::ReleaseExisting(const std::string& sPath) const
    {
        if (m_oContext.oManager.Unload(sPath))
        {
            ImportLog::Debug("Import '{}': replaced previously registered asset '{}'", m_oContext.sSourceAssetPath, sPath);
        }
    }
}
