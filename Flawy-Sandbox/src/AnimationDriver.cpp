#include "AnimationDriver.hpp"

#include <cmath>
#include <string_view>

namespace Sandbox
{
    namespace
    {
        constexpr std::string_view kEnergyParameter = "Energy";
        constexpr std::string_view kWaveTrigger = "Wave";
        constexpr double kWaveIntervalSeconds = 3.0;
        constexpr double kEnergyFrequency = 0.5;
    }

    void AnimationDriver::Update(AnimationResources& oResources, double dElapsedSeconds, double dDeltaSeconds)
    {
        Flawy::Animation::Animator& oAnimator = *oResources.upAnimator;
        oAnimator.SetFloat(kEnergyParameter, static_cast<float>(0.5 + 0.5 * std::sin(kEnergyFrequency * dElapsedSeconds)));
        if (dElapsedSeconds >= m_dNextWaveSeconds)
        {
            oAnimator.SetTrigger(kWaveTrigger);
            m_dNextWaveSeconds += kWaveIntervalSeconds;
        }

        oAnimator.Update(static_cast<float>(dDeltaSeconds));
        LogStateChange(oAnimator, dElapsedSeconds);
    }

    void AnimationDriver::LogStateChange(const Flawy::Animation::Animator& oAnimator, double dElapsedSeconds)
    {
        const uint32_t uActiveState = oAnimator.GetTargetStateIndex().value_or(oAnimator.GetCurrentStateIndex());
        if (m_optReportedState == uActiveState)
        {
            return;
        }

        const Flawy::Animation::AnimationController& oController = oAnimator.GetController();
        if (m_optReportedState)
        {
            Flawy::Core::Logger::Get().info("Animator state: {} -> {} at {:.2f} s",
                                            oController.GetState(*m_optReportedState).sName,
                                            oController.GetState(uActiveState).sName,
                                            dElapsedSeconds);
        }
        m_optReportedState = uActiveState;
    }
}
