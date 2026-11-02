#pragma once

#include "Flawy/Animation/AnimationControllerDescriptor.hpp"

#include <string_view>

namespace Flawy::Animation
{
    class AnimationParameterTypeName final
    {
    public:
        AnimationParameterTypeName() = delete;

        [[nodiscard]] static std::string_view Get(AnimationParameterType eType) noexcept;
    };
}
