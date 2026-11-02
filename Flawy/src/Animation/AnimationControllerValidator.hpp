#pragma once

#include "Flawy/Animation/AnimationControllerDescriptor.hpp"

#include <cstddef>
#include <string_view>

namespace Flawy::Animation
{
    class AnimationControllerValidator final
    {
    public:
        explicit AnimationControllerValidator(const AnimationControllerDescriptor& oDescriptor) noexcept;

        void Validate() const;

    private:
        void ValidateParameters() const;
        void ValidateParameter(const AnimationParameterDescriptor& oParameter) const;
        void ValidateStates() const;
        void ValidateState(const AnimationStateDescriptor& oState) const;
        void ValidateBlendSpace(const AnimationStateDescriptor& oState) const;
        void ValidateTransitions() const;
        void ValidateTransition(const AnimationTransitionDescriptor& oTransition, size_t uIndex) const;
        void ValidateCondition(const AnimationConditionDescriptor& oCondition, size_t uTransition, size_t uCondition) const;
        [[nodiscard]] const AnimationParameterDescriptor* FindParameter(std::string_view svName) const noexcept;
        [[nodiscard]] bool HasState(std::string_view svName) const noexcept;
        [[noreturn]] void Fail(std::string_view svReason) const;

        const AnimationControllerDescriptor& m_oDescriptor;
    };
}
