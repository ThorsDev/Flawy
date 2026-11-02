#pragma once

#include "Flawy/Animation/AnimationClipDescriptor.hpp"

#include <ufbx.h>

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace Flawy::Import
{
    class FbxSkinBinding;

    class FbxAnimationReader final
    {
    public:
        FbxAnimationReader(const ufbx_scene& oScene, const FbxSkinBinding& oSkinBinding, std::vector<std::string>& vWarnings);

        [[nodiscard]] std::optional<Animation::AnimationClipDescriptor> Read(const ufbx_anim_stack& oStack, size_t uStack);

    private:
        const ufbx_scene& m_oScene;
        const FbxSkinBinding& m_oSkinBinding;
        std::vector<std::string>& m_vWarnings;
    };
}
