#include "AnimationFactory.hpp"

#include <glm/ext/quaternion_trigonometric.hpp>
#include <glm/ext/scalar_constants.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <functional>
#include <string>

namespace Sandbox
{
    namespace
    {
        constexpr std::string_view kSkeletonPath = "generated/animation/SandboxArm.flskel";
        constexpr std::string_view kIdleClipPath = "generated/animation/Idle.flclip";
        constexpr std::string_view kSwayClipPath = "generated/animation/Sway.flclip";
        constexpr std::string_view kWaveClipPath = "generated/animation/Wave.flclip";
        constexpr std::string_view kControllerPath = "animation/SandboxArm.flctrl";
        constexpr std::array<std::string_view, 3> kClipPaths = {kIdleClipPath, kSwayClipPath, kWaveClipPath};

        constexpr std::string_view kSkeletonName = "SandboxArm";
        constexpr std::string_view kRootJoint = "Root";
        constexpr std::string_view kSpineJoint = "Spine";
        constexpr std::string_view kUpperArmJoint = "UpperArm";
        constexpr std::string_view kForearmJoint = "Forearm";
        constexpr glm::vec3 kSpineOffset = {0.0f, 0.4f, 0.0f};
        constexpr glm::vec3 kUpperArmOffset = {0.0f, 0.4f, 0.0f};
        constexpr glm::vec3 kForearmOffset = {0.0f, 0.35f, 0.0f};
        constexpr glm::vec3 kSwingAxis = {0.0f, 0.0f, 1.0f};

        constexpr float kLoopDurationSeconds = 2.0f;
        constexpr float kWaveDurationSeconds = 1.0f;
        constexpr float kIdleSpineDegrees = 5.0f;
        constexpr float kSwaySpineDegrees = 25.0f;
        constexpr float kSwayUpperArmDegrees = 20.0f;
        constexpr float kWaveForearmDegrees = 45.0f;
        constexpr uint32_t kKeyCount = 17;

        using AngleCurve = std::function<float(float)>;

        float FullSwing(float fPhase) noexcept
        {
            return std::sin(fPhase * glm::two_pi<float>());
        }

        float HalfSwing(float fPhase) noexcept
        {
            return std::sin(fPhase * glm::pi<float>());
        }

        Flawy::Animation::JointTransform MakeOffset(const glm::vec3& vecOffset)
        {
            Flawy::Animation::JointTransform oTransform;
            oTransform.vecTranslation = vecOffset;
            return oTransform;
        }

        void AddSwingTrack(Flawy::Animation::AnimationClipDescriptor& oClip,
                           std::string_view svJoint,
                           float fAmplitudeDegrees,
                           const AngleCurve& fnCurve)
        {
            Flawy::Animation::JointTrackDescriptor& oTrack = oClip.AddTrack(std::string(svJoint));
            for (uint32_t uKey = 0; uKey < kKeyCount; ++uKey)
            {
                const float fPhase = static_cast<float>(uKey) / static_cast<float>(kKeyCount - 1);
                const float fRadians = glm::radians(fAmplitudeDegrees * fnCurve(fPhase));
                oTrack.vRotations.push_back({fPhase * oClip.fDurationSeconds, glm::angleAxis(fRadians, kSwingAxis)});
            }
        }

        Flawy::Animation::AnimationClipDescriptor MakeClip(std::string_view svName, float fDurationSeconds)
        {
            Flawy::Animation::AnimationClipDescriptor oClip;
            oClip.sName = std::string(svName);
            oClip.sSkeletonPath = std::string(kSkeletonPath);
            oClip.fDurationSeconds = fDurationSeconds;
            return oClip;
        }
    }

    void AnimationFactory::Create(AnimationResources& oResources, Flawy::Assets::AssetManager& oAssetManager)
    {
        EnsureGeneratedAssets(oAssetManager);

        oResources.oSkeleton = oAssetManager.Load<Flawy::Animation::SkeletonAsset>(kSkeletonPath);
        oResources.oController = oAssetManager.Load<Flawy::Animation::AnimationControllerAsset>(kControllerPath);

        const Flawy::Animation::Skeleton& oSkeleton = oResources.oSkeleton->GetSkeleton();
        oResources.upClipLibrary = std::make_unique<Flawy::Animation::AnimationClipLibrary>(oSkeleton);
        for (const std::string_view svClipPath : kClipPaths)
        {
            static_cast<void>(oResources.upClipLibrary->AddAsset(oAssetManager.Load<Flawy::Animation::AnimationClipAsset>(svClipPath)));
        }

        const Flawy::Animation::AnimationController& oController = oResources.oController->GetController();
        oResources.upAnimator = std::make_unique<Flawy::Animation::Animator>(oController, oSkeleton, *oResources.upClipLibrary);
        Flawy::Core::Logger::Get().info("Animation: skeleton '{}' with {} joint(s), {} clip(s), controller '{}' entering '{}'",
                                        oSkeleton.GetName(),
                                        oSkeleton.GetJointCount(),
                                        oResources.upClipLibrary->GetClipCount(),
                                        oController.GetName(),
                                        oResources.upAnimator->GetCurrentStateName());
    }

    void AnimationFactory::Release(AnimationResources& oResources) noexcept
    {
        oResources.upAnimator.reset();
        oResources.upClipLibrary.reset();
        oResources.oController.Reset();
        oResources.oSkeleton.Reset();
    }

    void AnimationFactory::EnsureGeneratedAssets(Flawy::Assets::AssetManager& oAssetManager)
    {
        SaveSkeletonIfMissing(oAssetManager, kSkeletonPath);
        SaveClipIfMissing(oAssetManager, kIdleClipPath, BuildIdleClip());
        SaveClipIfMissing(oAssetManager, kSwayClipPath, BuildSwayClip());
        SaveClipIfMissing(oAssetManager, kWaveClipPath, BuildWaveClip());
    }

    void AnimationFactory::SaveSkeletonIfMissing(Flawy::Assets::AssetManager& oAssetManager, std::string_view svPath)
    {
        const std::filesystem::path oPath = oAssetManager.ResolvePath(svPath);
        if (std::filesystem::exists(oPath))
        {
            return;
        }
        Flawy::Animation::SkeletonDescriptorSerializer::Save(oPath, BuildSkeleton());
        Flawy::Core::Logger::Get().info("Saved generated asset '{}'", svPath);
    }

    void AnimationFactory::SaveClipIfMissing(Flawy::Assets::AssetManager& oAssetManager,
                                             std::string_view svPath,
                                             const Flawy::Animation::AnimationClipDescriptor& oClip)
    {
        const std::filesystem::path oPath = oAssetManager.ResolvePath(svPath);
        if (std::filesystem::exists(oPath))
        {
            return;
        }
        Flawy::Animation::AnimationClipDescriptorSerializer::Save(oPath, oClip);
        Flawy::Core::Logger::Get().info("Saved generated asset '{}'", svPath);
    }

    Flawy::Animation::SkeletonDescriptor AnimationFactory::BuildSkeleton()
    {
        Flawy::Animation::SkeletonDescriptor oSkeleton;
        oSkeleton.sName = std::string(kSkeletonName);
        const uint32_t uRoot = oSkeleton.AddJoint(std::string(kRootJoint), Flawy::Animation::SkeletonDescriptor::kNoParent, {});
        const uint32_t uSpine = oSkeleton.AddJoint(std::string(kSpineJoint), static_cast<int32_t>(uRoot), MakeOffset(kSpineOffset));
        const uint32_t uUpperArm =
            oSkeleton.AddJoint(std::string(kUpperArmJoint), static_cast<int32_t>(uSpine), MakeOffset(kUpperArmOffset));
        static_cast<void>(oSkeleton.AddJoint(std::string(kForearmJoint), static_cast<int32_t>(uUpperArm), MakeOffset(kForearmOffset)));
        return oSkeleton;
    }

    Flawy::Animation::AnimationClipDescriptor AnimationFactory::BuildIdleClip()
    {
        Flawy::Animation::AnimationClipDescriptor oClip = MakeClip("Idle", kLoopDurationSeconds);
        AddSwingTrack(oClip, kSpineJoint, kIdleSpineDegrees, FullSwing);
        return oClip;
    }

    Flawy::Animation::AnimationClipDescriptor AnimationFactory::BuildSwayClip()
    {
        Flawy::Animation::AnimationClipDescriptor oClip = MakeClip("Sway", kLoopDurationSeconds);
        AddSwingTrack(oClip, kSpineJoint, kSwaySpineDegrees, FullSwing);
        AddSwingTrack(oClip, kUpperArmJoint, kSwayUpperArmDegrees, FullSwing);
        return oClip;
    }

    Flawy::Animation::AnimationClipDescriptor AnimationFactory::BuildWaveClip()
    {
        Flawy::Animation::AnimationClipDescriptor oClip = MakeClip("Wave", kWaveDurationSeconds);
        AddSwingTrack(oClip, kForearmJoint, kWaveForearmDegrees, HalfSwing);
        return oClip;
    }
}
