#include "ImportFactory.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/vec3.hpp>

#include <filesystem>
#include <format>
#include <memory>
#include <string>
#include <utility>

namespace Sandbox
{
    namespace
    {
        constexpr std::string_view kCubeSourcePath = "models/Cube.obj";
        constexpr std::string_view kTubeSourcePath = "generated/models/Tube.glb";
        constexpr std::string_view kImportOutputDirectory = "generated/imported";
        constexpr std::string_view kTubeImportDirectory = "generated/imported/Tube";
        constexpr std::string_view kControllerName = "ImportedClip";
        constexpr std::string_view kPlayStateName = "Play";
        constexpr glm::vec3 kCubePosition = {2.0f, -0.65f, 0.3f};
        constexpr glm::vec3 kTubePosition = {1.6f, -0.9f, -1.4f};
        constexpr glm::vec3 kTubeJointOrigin = {2.1f, -0.9f, -1.4f};
        constexpr glm::vec3 kCustomPosition = {0.0f, -0.9f, -1.5f};

        std::string MakeTubeAssetPath(std::string_view svName, std::string_view svExtension)
        {
            return std::format("{}/{}{}", kTubeImportDirectory, svName, svExtension);
        }

        glm::mat4 MakePlacement(const glm::vec3& vecPosition)
        {
            return glm::translate(glm::mat4(1.0f), vecPosition);
        }

        template <typename TAsset>
        Flawy::Assets::AssetHandle<TAsset> FindFirst(const Flawy::Assets::AssetImportResult& oResult)
        {
            for (const std::shared_ptr<Flawy::Assets::Asset>& spAsset : oResult.vAssets)
            {
                if (spAsset->GetTypeId() == TAsset::kTypeId)
                {
                    return Flawy::Assets::AssetHandle<TAsset>(std::static_pointer_cast<TAsset>(spAsset));
                }
            }
            return {};
        }
    }

    void ImportFactory::Create(ImportResources& oResources, Flawy::Assets::AssetManager& oAssetManager, const std::optional<std::string>& optImportPath)
    {
        if (optImportPath)
        {
            CreateCustomScene(oResources, oAssetManager, *optImportPath);
            return;
        }
        CreateDefaultScene(oResources, oAssetManager);
    }

    void ImportFactory::Update(ImportResources& oResources, double dDeltaSeconds)
    {
        if (oResources.upAnimator)
        {
            oResources.upAnimator->Update(static_cast<float>(dDeltaSeconds));
        }
    }

    void ImportFactory::Release(ImportResources& oResources) noexcept
    {
        oResources.upAnimator.reset();
        oResources.upClipLibrary.reset();
        oResources.upController.reset();
        oResources.oSkeleton.Reset();
        oResources.vModels.clear();
    }

    void ImportFactory::CreateDefaultScene(ImportResources& oResources, Flawy::Assets::AssetManager& oAssetManager)
    {
        const Flawy::Assets::AssetImportResult oCubeResult = oAssetManager.Import(kCubeSourcePath, Flawy::Assets::AssetImportOptions{});
        AddModels(oResources, oCubeResult, MakePlacement(kCubePosition));

        EnsureSampleTube(oAssetManager);
        Flawy::Assets::AssetImportOptions oTubeOptions;
        oTubeOptions.bSaveToAssetRoot = true;
        oTubeOptions.sOutputDirectory = std::string(kImportOutputDirectory);
        const std::vector<std::string> vSavedPaths = oAssetManager.Import(kTubeSourcePath, oTubeOptions).vSavedPaths;
        for (const std::string& sPath : vSavedPaths)
        {
            static_cast<void>(oAssetManager.Unload(sPath));
        }

        const std::string sMeshPath = MakeTubeAssetPath(Flawy::Import::SampleModelWriter::kTubeMeshName, Flawy::Assets::StaticMeshAsset::kExtension);
        const std::string sSkeletonPath = MakeTubeAssetPath(Flawy::Import::SampleModelWriter::kTubeSkeletonName, Flawy::Animation::SkeletonAsset::kExtension);
        const std::string sClipPath = MakeTubeAssetPath(Flawy::Import::SampleModelWriter::kTubeClipName, Flawy::Animation::AnimationClipAsset::kExtension);
        oResources.vModels.push_back({oAssetManager.Load<Flawy::Assets::StaticMeshAsset>(sMeshPath), MakePlacement(kTubePosition)});
        CreateAnimator(oResources,
                       oAssetManager.Load<Flawy::Animation::SkeletonAsset>(sSkeletonPath),
                       oAssetManager.Load<Flawy::Animation::AnimationClipAsset>(sClipPath),
                       MakePlacement(kTubeJointOrigin));
    }

    void ImportFactory::CreateCustomScene(ImportResources& oResources, Flawy::Assets::AssetManager& oAssetManager, std::string_view svSourcePath)
    {
        const Flawy::Assets::AssetImportResult oResult = oAssetManager.Import(svSourcePath, Flawy::Assets::AssetImportOptions{});
        AddModels(oResources, oResult, MakePlacement(kCustomPosition));

        Flawy::Assets::AssetHandle<Flawy::Animation::SkeletonAsset> oSkeleton = FindFirst<Flawy::Animation::SkeletonAsset>(oResult);
        Flawy::Assets::AssetHandle<Flawy::Animation::AnimationClipAsset> oClip = FindFirst<Flawy::Animation::AnimationClipAsset>(oResult);
        if (oSkeleton.IsValid() && oClip.IsValid())
        {
            CreateAnimator(oResources, std::move(oSkeleton), std::move(oClip), MakePlacement(kCustomPosition));
        }
    }

    void ImportFactory::EnsureSampleTube(Flawy::Assets::AssetManager& oAssetManager)
    {
        const std::filesystem::path oPath = oAssetManager.ResolvePath(kTubeSourcePath);
        if (std::filesystem::exists(oPath))
        {
            return;
        }
        Flawy::Import::SampleModelWriter::WriteSkinnedTubeGlb(oPath);
        Flawy::Core::Logger::Get().info("Saved generated asset '{}'", kTubeSourcePath);
    }

    void ImportFactory::AddModels(ImportResources& oResources, const Flawy::Assets::AssetImportResult& oResult, const glm::mat4& matModel)
    {
        for (const std::shared_ptr<Flawy::Assets::Asset>& spAsset : oResult.vAssets)
        {
            if (spAsset->GetTypeId() == Flawy::Assets::StaticMeshAsset::kTypeId)
            {
                oResources.vModels.push_back({Flawy::Assets::AssetHandle<Flawy::Assets::StaticMeshAsset>(std::static_pointer_cast<Flawy::Assets::StaticMeshAsset>(spAsset)), matModel});
            }
        }
    }

    void ImportFactory::CreateAnimator(ImportResources& oResources,
                                       Flawy::Assets::AssetHandle<Flawy::Animation::SkeletonAsset> oSkeleton,
                                       Flawy::Assets::AssetHandle<Flawy::Animation::AnimationClipAsset> oClip,
                                       const glm::mat4& matJointOrigin)
    {
        const Flawy::Animation::Skeleton& oRuntimeSkeleton = oSkeleton->GetSkeleton();
        const std::string sClipName = oClip->GetClip().GetName();
        const float fClipDuration = oClip->GetClip().GetDurationSeconds();

        Flawy::Animation::AnimationControllerDescriptor oDescriptor;
        oDescriptor.sName = std::string(kControllerName);
        oDescriptor.sEntryState = std::string(kPlayStateName);
        Flawy::Animation::AnimationStateDescriptor oState;
        oState.sName = std::string(kPlayStateName);
        oState.sClipName = sClipName;
        oDescriptor.vStates.push_back(std::move(oState));

        oResources.oSkeleton = std::move(oSkeleton);
        oResources.upController = std::make_unique<Flawy::Animation::AnimationController>(oDescriptor);
        oResources.upClipLibrary = std::make_unique<Flawy::Animation::AnimationClipLibrary>(oRuntimeSkeleton);
        static_cast<void>(oResources.upClipLibrary->AddAsset(std::move(oClip)));
        oResources.upAnimator = std::make_unique<Flawy::Animation::Animator>(*oResources.upController, oRuntimeSkeleton, *oResources.upClipLibrary);
        oResources.matJointOrigin = matJointOrigin;
        Flawy::Core::Logger::Get().info("Imported animation: skeleton '{}' with {} joint(s), clip '{}' ({:.2f} s) playing in state '{}'",
                                        oRuntimeSkeleton.GetName(),
                                        oRuntimeSkeleton.GetJointCount(),
                                        sClipName,
                                        fClipDuration,
                                        oResources.upAnimator->GetCurrentStateName());
    }
}
