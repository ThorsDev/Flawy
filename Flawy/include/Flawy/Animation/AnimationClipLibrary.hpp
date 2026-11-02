#pragma once

#include "Flawy/Animation/AnimationClipAsset.hpp"
#include "Flawy/Animation/IAnimationClipProvider.hpp"
#include "Flawy/Assets/AssetHandle.hpp"
#include "Flawy/Core/StringHash.hpp"

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Flawy::Animation
{
    class AnimationClip;
    class Skeleton;

    class AnimationClipLibrary final : public IAnimationClipProvider
    {
    public:
        explicit AnimationClipLibrary(const Skeleton& oSkeleton);
        ~AnimationClipLibrary() override;

        AnimationClipLibrary(const AnimationClipLibrary&) = delete;
        AnimationClipLibrary& operator=(const AnimationClipLibrary&) = delete;
        AnimationClipLibrary(AnimationClipLibrary&&) = delete;
        AnimationClipLibrary& operator=(AnimationClipLibrary&&) = delete;

        const AnimationClip& Add(std::unique_ptr<AnimationClip> upClip);
        const AnimationClip& AddAsset(Assets::AssetHandle<AnimationClipAsset> oClipAsset);

        [[nodiscard]] const AnimationClip* FindClip(std::string_view svName) const noexcept override;
        [[nodiscard]] uint32_t GetClipCount() const noexcept;
        [[nodiscard]] const Skeleton& GetSkeleton() const noexcept;

    private:
        void RequireAddable(const AnimationClip& oClip) const;

        const Skeleton& m_oSkeleton;
        std::vector<std::unique_ptr<AnimationClip>> m_vOwnedClips;
        std::vector<Assets::AssetHandle<AnimationClipAsset>> m_vClipAssets;
        std::unordered_map<std::string, const AnimationClip*, Core::TransparentStringHash, std::equal_to<>> m_mapClips;
    };
}
