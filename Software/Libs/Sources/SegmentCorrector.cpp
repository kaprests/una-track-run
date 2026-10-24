#include "SegmentCorrector.hpp"

namespace {
constexpr float square(float x) { return x * x; }
} // namespace

bool SegmentCorrector::anchor(float rawM, float knownM, float durationS)
{
    using C = Config;

    const bool runningSegment = durationS > 0.0f && rawM >= C::kMinRawM && rawM / durationS >= C::kMinSpeedMps;
    if (!runningSegment || knownM <= 0.0f) {
        return false;
    }

    const float measured = knownM / rawM;
    if (measured < C::kMinK || measured > C::kMaxK) {
        return false;
    }

    const float measurementVariance = C::kMeasurementVariance * C::kRefSegmentM / knownM;
    const float innovation          = measured - mState.k;
    const float innovationVariance  = mState.variance + measurementVariance;
    if (square(innovation) > square(C::kGateSigmas) * innovationVariance) {
        return false;
    }

    const float gain = mState.variance / innovationVariance;
    mState.k        += gain * innovation;
    mState.variance *= 1.0f - gain;
    return true;
}
