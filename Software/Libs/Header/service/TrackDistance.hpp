#ifndef TRACK_DISTANCE_HPP
#define TRACK_DISTANCE_HPP

#include <algorithm>
#include <optional>

/**
 * Corrected distance of a session: the distance fixed at the last segment end
 * plus the model's distance since then, scaled by k. The result never decreases.
 */
class TrackDistance {
public:
    void reset() { *this = TrackDistance{}; }

    float total(float rawTotalM, float k)
    {
        mHighestM = std::max(mHighestM, estimate(rawTotalM, k));
        return mHighestM;
    }

    /// Ends the current segment; a known length fixes the distance, otherwise the estimate carries on.
    void closeSegment(float rawTotalM, float k, std::optional<float> knownM)
    {
        mSegmentStartM      = knownM ? mSegmentStartM + *knownM : estimate(rawTotalM, k);
        mRawAtSegmentStartM = rawTotalM;
        mHighestM           = std::max(mHighestM, mSegmentStartM);
    }

    /// The model's distance since the segment started.
    float rawSegmentM(float rawTotalM) const { return rawTotalM - mRawAtSegmentStartM; }

private:
    float estimate(float rawTotalM, float k) const { return mSegmentStartM + k * rawSegmentM(rawTotalM); }

    float mSegmentStartM      = 0.0f;
    float mRawAtSegmentStartM = 0.0f;
    float mHighestM           = 0.0f;
};

#endif // TRACK_DISTANCE_HPP
