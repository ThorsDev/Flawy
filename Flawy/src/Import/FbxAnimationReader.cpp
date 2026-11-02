#include "Import/FbxAnimationReader.hpp"

#include "Import/FbxSkinBinding.hpp"
#include "Import/SkeletonAssembler.hpp"
#include "Import/UfbxConversion.hpp"

#include <glm/ext/quaternion_geometric.hpp>

#include <algorithm>
#include <cstdint>
#include <format>
#include <limits>
#include <map>
#include <memory>
#include <utility>

namespace Flawy::Import
{
    namespace
    {
        constexpr float kMinimumClipDuration = 1.0f / 30.0f;

        struct UfbxBakedAnimDeleter
        {
            void operator()(ufbx_baked_anim* pBake) const noexcept
            {
                ufbx_free_baked_anim(pBake);
            }
        };

        template <typename TKey, typename TSource, typename TConvert>
        std::vector<TKey> ConvertKeys(const TSource& oSource, double dStart, float fDuration, TConvert&& fnConvert)
        {
            std::vector<TKey> vKeys;
            vKeys.reserve(oSource.count);
            for (size_t uKey = 0; uKey < oSource.count; ++uKey)
            {
                const auto& oKey = oSource.data[uKey];
                const auto fTime = static_cast<float>(std::max(oKey.time - dStart, 0.0));
                if ((!vKeys.empty() && fTime <= vKeys.back().fTimeSeconds) || fTime > fDuration)
                {
                    continue;
                }
                vKeys.push_back({fTime, fnConvert(oKey.value)});
            }
            return vKeys;
        }

        template <typename TKeys>
        void ExtendRange(const TKeys& oKeys, double& dStart, double& dEnd) noexcept
        {
            if (oKeys.count == 0)
            {
                return;
            }
            dStart = std::min(dStart, oKeys.data[0].time);
            dEnd = std::max(dEnd, oKeys.data[oKeys.count - 1].time);
        }
    }

    FbxAnimationReader::FbxAnimationReader(const ufbx_scene& oScene, const FbxSkinBinding& oSkinBinding, std::vector<std::string>& vWarnings)
        : m_oScene(oScene)
        , m_oSkinBinding(oSkinBinding)
        , m_vWarnings(vWarnings)
    {
    }

    std::optional<Animation::AnimationClipDescriptor> FbxAnimationReader::Read(const ufbx_anim_stack& oStack, size_t uStack)
    {
        const std::string sSourceName = UfbxConversion::ToString(oStack.name);
        const std::string sName = sSourceName.empty() ? std::format("Take{}", uStack) : sSourceName;

        ufbx_bake_opts oBakeOptions{};
        oBakeOptions.trim_start_time = true;
        ufbx_error oError{};
        const std::unique_ptr<ufbx_baked_anim, UfbxBakedAnimDeleter> upBake(ufbx_bake_anim(&m_oScene, oStack.anim, &oBakeOptions, &oError));
        if (!upBake)
        {
            m_vWarnings.push_back(std::format("animation '{}' could not be baked and was skipped: {}", sName, UfbxConversion::FormatError(oError)));
            return std::nullopt;
        }

        double dStart = std::numeric_limits<double>::max();
        double dEnd = std::numeric_limits<double>::lowest();
        std::map<uint32_t, const ufbx_baked_node*> mapJointNodes;
        for (size_t uNode = 0; uNode < upBake->nodes.count; ++uNode)
        {
            const ufbx_baked_node& oBaked = upBake->nodes.data[uNode];
            if (oBaked.typed_id >= m_oScene.nodes.count)
            {
                continue;
            }
            const std::optional<uint32_t> optJoint = m_oSkinBinding.FindJoint(m_oScene.nodes.data[oBaked.typed_id]);
            if (!optJoint)
            {
                continue;
            }
            mapJointNodes.emplace(*optJoint, &oBaked);
            ExtendRange(oBaked.translation_keys, dStart, dEnd);
            ExtendRange(oBaked.rotation_keys, dStart, dEnd);
            ExtendRange(oBaked.scale_keys, dStart, dEnd);
        }
        if (mapJointNodes.empty() || dEnd < dStart)
        {
            m_vWarnings.push_back(std::format("animation '{}' does not animate any skeleton joint and was skipped", sName));
            return std::nullopt;
        }

        const SkeletonAssembler& oSkeleton = m_oSkinBinding.GetSkeleton();
        Animation::AnimationClipDescriptor oClip;
        oClip.sName = sName;
        oClip.fDurationSeconds = std::max(static_cast<float>(dEnd - dStart), kMinimumClipDuration);
        for (const auto& [uJoint, pBaked] : mapJointNodes)
        {
            Animation::JointTrackDescriptor& oTrack = oClip.AddTrack(oSkeleton.GetJointName(uJoint));
            const uint32_t uSourceJoint = uJoint;
            oTrack.vTranslations = ConvertKeys<Animation::Vec3Key>(pBaked->translation_keys, dStart, oClip.fDurationSeconds, [&oSkeleton, uSourceJoint](const ufbx_vec3& vecValue) {
                return oSkeleton.ToJointTranslation(uSourceJoint, UfbxConversion::ToVector(vecValue));
            });
            oTrack.vRotations = ConvertKeys<Animation::QuatKey>(pBaked->rotation_keys, dStart, oClip.fDurationSeconds, [&oSkeleton, uSourceJoint](const ufbx_quat& quatValue) {
                return oSkeleton.ToJointRotation(uSourceJoint, glm::normalize(UfbxConversion::ToQuaternion(quatValue)));
            });
            oTrack.vScales = ConvertKeys<Animation::Vec3Key>(pBaked->scale_keys, dStart, oClip.fDurationSeconds, [&oSkeleton, uSourceJoint](const ufbx_vec3& vecValue) {
                return oSkeleton.ToJointScale(uSourceJoint, UfbxConversion::ToVector(vecValue));
            });
        }
        return oClip;
    }
}
