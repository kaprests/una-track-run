#ifndef SEGMENT_CORRECTOR_HPP
#define SEGMENT_CORRECTOR_HPP

/**
 * Scalar Kalman filter on the factor k that scales the distance the cadence
 * model integrates to the real distance. Each known-distance segment (a track
 * lap or an interval rep) is one measurement of k.
 */
class SegmentCorrector {
public:
    struct Config {
        static constexpr float kPriorVariance       = 2.5e-3f; ///< 5 % standard deviation
        static constexpr float kMeasurementVariance = 9.0e-4f; ///< 3 % at kRefSegmentM
        static constexpr float kRefSegmentM         = 200.0f;
        static constexpr float kProcessVariancePerS = 1.0e-6f; ///< fatigue drift
        static constexpr float kGateSigmas          = 3.0f;
        static constexpr float kMinK                = 0.8f;
        static constexpr float kMaxK                = 1.25f;
        static constexpr float kMinRawM             = 20.0f;
        static constexpr float kMinSpeedMps         = 1.5f;    ///< slower segments are not running
    };

    struct State {
        float k        = 1.0f;
        float variance = Config::kPriorVariance;
    };

    SegmentCorrector() = default;
    explicit SegmentCorrector(State prior) : mState(prior) {}

    /// Folds one segment in. Returns false, leaving k unchanged, if the segment is not trustworthy.
    bool anchor(float rawM, float knownM, float durationS);

    void drift(float elapsedS) { mState.variance += Config::kProcessVariancePerS * elapsedS; }

    float        k() const        { return mState.k; }
    const State& state() const    { return mState; }

private:
    State mState;
};

#endif // SEGMENT_CORRECTOR_HPP
