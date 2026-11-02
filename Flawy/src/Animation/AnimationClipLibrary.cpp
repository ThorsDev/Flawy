#include "Flawy/Animation/AnimationClipLibrary.hpp"

#include "Flawy/Animation/AnimationClip.hpp"
#include "Flawy/Animation/Skeleton.hpp"

#include <format>
#include <stdexcept>
#include <utility>

namespace Flawy::Animation
{
    AnimationClipLibrary::AnimationClipLibrary(const Skeleton& oSkeleton)
        : m_oSkeleton(oSkeleton)
    {
    }

    AnimationClipLibrary::~AnimationClipLibrary() = default;

    const AnimationClip& AnimationClipLibrary::Add(std::unique_ptr<AnimationClip> upClip)
    {
        if (!upClip)
        {
            throw std::invalid_argument("Cannot add a null animation clip to the clip library");
        }
        RequireAddable(*upClip);

        const AnimationClip& oClip = *upClip;
        m_vOwnedClips.push_back(std::move(upClip));
        m_mapClips.emplace(oClip.GetName(), &oClip);
        return oClip;
    }

    const AnimationClip& AnimationClipLibrary::AddAsset(Assets::AssetHandle<AnimationClipAsset> oClipAsset)
    {
        if (!oClipAsset.IsValid())
        {
            throw std::invalid_argument("Cannot add an empty animation clip asset handle to the clip library");
        }
        const AnimationClip& oClip = oClipAsset->GetClip();
        RequireAddable(oClip);

        m_vClipAssets.push_back(std::move(oClipAsset));
        m_mapClips.emplace(oClip.GetName(), &oClip);
        return oClip;
    }

    const AnimationClip* AnimationClipLibrary::FindClip(std::string_view svName) const noexcept
    {
        const auto itClip = m_mapClips.find(svName);
        return itClip == m_mapClips.end() ? nullptr : itClip->second;
    }

    uint32_t AnimationClipLibrary::GetClipCount() const noexcept
    {
        return static_cast<uint32_t>(m_mapClips.size());
    }

    const Skeleton& AnimationClipLibrary::GetSkeleton() const noexcept
    {
        return m_oSkeleton;
    }

    void AnimationClipLibrary::RequireAddable(const AnimationClip& oClip) const
    {
        if (&oClip.GetSkeleton() != &m_oSkeleton)
        {
            throw std::invalid_argument(std::format("Animation clip '{}' is bound to skeleton '{}' but the clip library uses skeleton '{}'",
                                                    oClip.GetName(),
                                                    oClip.GetSkeleton().GetName(),
                                                    m_oSkeleton.GetName()));
        }
        if (m_mapClips.contains(oClip.GetName()))
        {
            throw std::invalid_argument(std::format("The clip library already contains an animation clip named '{}'", oClip.GetName()));
        }
    }
}
