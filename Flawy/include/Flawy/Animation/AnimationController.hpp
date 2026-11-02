#pragma once

#include "Flawy/Animation/AnimationControllerDescriptor.hpp"
#include "Flawy/Core/StringHash.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace Flawy::Animation
{
    struct AnimationParameter
    {
        std::string sName;
        AnimationParameterType eType = AnimationParameterType::Float;
        float fDefaultValue = 0.0f;
        bool bDefaultValue = false;
    };

    struct AnimationBlendSpaceEntry
    {
        float fThreshold = 0.0f;
        std::string sClipName;
    };

    struct AnimationState
    {
        std::string sName;
        std::optional<uint32_t> optBlendParameter;
        std::vector<AnimationBlendSpaceEntry> vEntries;
        float fSpeed = 1.0f;
        bool bLoop = true;
    };

    struct AnimationCondition
    {
        uint32_t uParameter = 0;
        AnimationConditionOperator eOperator = AnimationConditionOperator::Triggered;
        float fThreshold = 0.0f;
    };

    struct AnimationTransition
    {
        std::optional<uint32_t> optFromState;
        uint32_t uToState = 0;
        float fDurationSeconds = AnimationTransitionDescriptor::kDefaultDurationSeconds;
        std::optional<float> optExitTime;
        std::vector<AnimationCondition> vConditions;
    };

    class AnimationController final
    {
    public:
        explicit AnimationController(const AnimationControllerDescriptor& oDescriptor);

        [[nodiscard]] const std::string& GetName() const noexcept;
        [[nodiscard]] uint32_t GetParameterCount() const noexcept;
        [[nodiscard]] const AnimationParameter& GetParameter(uint32_t uParameter) const;
        [[nodiscard]] std::optional<uint32_t> FindParameter(std::string_view svName) const noexcept;
        [[nodiscard]] uint32_t GetStateCount() const noexcept;
        [[nodiscard]] const AnimationState& GetState(uint32_t uState) const;
        [[nodiscard]] std::optional<uint32_t> FindState(std::string_view svName) const noexcept;
        [[nodiscard]] uint32_t GetEntryStateIndex() const noexcept;
        [[nodiscard]] std::span<const AnimationTransition> GetTransitions() const noexcept;

    private:
        using IndexMap = std::unordered_map<std::string, uint32_t, Core::TransparentStringHash, std::equal_to<>>;

        void ResolveParameters(const AnimationControllerDescriptor& oDescriptor);
        void ResolveStates(const AnimationControllerDescriptor& oDescriptor);
        void ResolveTransitions(const AnimationControllerDescriptor& oDescriptor);
        [[nodiscard]] static std::optional<uint32_t> FindIndex(const IndexMap& mapIndices, std::string_view svName) noexcept;

        std::string m_sName;
        std::vector<AnimationParameter> m_vParameters;
        std::vector<AnimationState> m_vStates;
        std::vector<AnimationTransition> m_vTransitions;
        IndexMap m_mapParameters;
        IndexMap m_mapStates;
        uint32_t m_uEntryState = 0;
    };
}
