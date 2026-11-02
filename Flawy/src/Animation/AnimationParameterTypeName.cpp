#include "Animation/AnimationParameterTypeName.hpp"

namespace Flawy::Animation
{
    std::string_view AnimationParameterTypeName::Get(AnimationParameterType eType) noexcept
    {
        switch (eType)
        {
        case AnimationParameterType::Float:
            return "float";
        case AnimationParameterType::Bool:
            return "bool";
        case AnimationParameterType::Trigger:
            return "trigger";
        }
        return "unknown";
    }
}
