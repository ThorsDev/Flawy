#pragma once

#include "Flawy/Animation/AnimationClipDescriptor.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <format>
#include <stdexcept>
#include <vector>

namespace Flawy::Import
{
    template <typename TValue>
    struct TimedValue
    {
        float fTime = 0.0f;
        TValue oValue{};
    };

    class KeyframeResampler final
    {
    public:
        static constexpr float kCubicSplineSampleRate = 30.0f;
        static constexpr float kStepEpsilonSeconds = 1.0e-4f;

        KeyframeResampler() = delete;

        template <typename TValue>
        [[nodiscard]] static std::vector<TimedValue<TValue>> CopyLinear(const std::vector<float>& vTimes, const std::vector<TValue>& vValues)
        {
            std::vector<TimedValue<TValue>> vKeys;
            vKeys.reserve(vTimes.size());
            for (size_t uKey = 0; uKey < vTimes.size(); ++uKey)
            {
                vKeys.push_back({vTimes[uKey], vValues[uKey]});
            }
            return vKeys;
        }

        template <typename TValue>
        [[nodiscard]] static std::vector<TimedValue<TValue>> ExpandStep(const std::vector<float>& vTimes, const std::vector<TValue>& vValues)
        {
            std::vector<TimedValue<TValue>> vKeys;
            vKeys.reserve(vTimes.size() * 2);
            for (size_t uKey = 0; uKey < vTimes.size(); ++uKey)
            {
                if (uKey != 0)
                {
                    const float fEpsilon = std::min(kStepEpsilonSeconds, 0.5f * (vTimes[uKey] - vTimes[uKey - 1]));
                    vKeys.push_back({vTimes[uKey] - fEpsilon, vValues[uKey - 1]});
                }
                vKeys.push_back({vTimes[uKey], vValues[uKey]});
            }
            return vKeys;
        }

        template <typename TValue>
        [[nodiscard]] static std::vector<TimedValue<TValue>> ResampleCubicSpline(const std::vector<float>& vTimes, const std::vector<TValue>& vValues)
        {
            const auto fnInTangent = [&vValues](size_t uKey) -> const TValue& { return vValues[uKey * 3]; };
            const auto fnValue = [&vValues](size_t uKey) -> const TValue& { return vValues[uKey * 3 + 1]; };
            const auto fnOutTangent = [&vValues](size_t uKey) -> const TValue& { return vValues[uKey * 3 + 2]; };

            std::vector<TimedValue<TValue>> vKeys;
            if (vTimes.size() == 1)
            {
                vKeys.push_back({vTimes.front(), fnValue(0)});
                return vKeys;
            }

            const float fStart = vTimes.front();
            const float fEnd = vTimes.back();
            const double dSamples = std::ceil((static_cast<double>(fEnd) - static_cast<double>(fStart)) * static_cast<double>(kCubicSplineSampleRate));
            if (!(dSamples + 1.0 <= static_cast<double>(Animation::AnimationClipDescriptor::kMaxKeysPerComponent)))
            {
                throw std::runtime_error(std::format("cubic spline resampling of a {} s channel at {} Hz exceeds {} keys",
                                                     fEnd - fStart,
                                                     kCubicSplineSampleRate,
                                                     Animation::AnimationClipDescriptor::kMaxKeysPerComponent));
            }
            const auto uSamples = static_cast<size_t>(dSamples);
            size_t uSegment = 0;
            for (size_t uSample = 0; uSample <= uSamples; ++uSample)
            {
                const float fTime = uSample == uSamples ? fEnd : std::min(fStart + static_cast<float>(uSample) / kCubicSplineSampleRate, fEnd);
                if (!vKeys.empty() && fTime <= vKeys.back().fTime)
                {
                    continue;
                }
                while (uSegment + 2 < vTimes.size() && fTime > vTimes[uSegment + 1])
                {
                    ++uSegment;
                }
                const float fDelta = vTimes[uSegment + 1] - vTimes[uSegment];
                const float fFactor = fDelta > 0.0f ? std::clamp((fTime - vTimes[uSegment]) / fDelta, 0.0f, 1.0f) : 0.0f;
                vKeys.push_back({fTime, EvaluateHermite(fnValue(uSegment), fnOutTangent(uSegment), fnValue(uSegment + 1), fnInTangent(uSegment + 1), fDelta, fFactor)});
            }
            return vKeys;
        }

    private:
        template <typename TValue>
        [[nodiscard]] static TValue EvaluateHermite(const TValue& oStart, const TValue& oStartTangent, const TValue& oEnd, const TValue& oEndTangent, float fDelta, float fFactor)
        {
            const float fFactor2 = fFactor * fFactor;
            const float fFactor3 = fFactor2 * fFactor;
            const float fH00 = 2.0f * fFactor3 - 3.0f * fFactor2 + 1.0f;
            const float fH10 = fFactor3 - 2.0f * fFactor2 + fFactor;
            const float fH01 = -2.0f * fFactor3 + 3.0f * fFactor2;
            const float fH11 = fFactor3 - fFactor2;
            return fH00 * oStart + fH10 * fDelta * oStartTangent + fH01 * oEnd + fH11 * fDelta * oEndTangent;
        }
    };
}
