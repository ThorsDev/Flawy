#pragma once

#include "AnimationResources.hpp"

#include <cstdint>
#include <optional>

namespace Sandbox
{
    class AnimationDriver final
    {
    public:
        void Update(AnimationResources& oResources, double dElapsedSeconds, double dDeltaSeconds);

    private:
        void LogStateChange(const Flawy::Animation::Animator& oAnimator, double dElapsedSeconds);

        double m_dNextWaveSeconds = 1.0;
        std::optional<uint32_t> m_optReportedState;
    };
}
