#ifndef TRACK_DISTANCE_HPP
#define TRACK_DISTANCE_HPP

#include <algorithm>
#include <cmath>
#include <optional>

/**
 * Corrected distance of a session. A segment of known length (a track lap or an
 * interval rep) holds the distance at its end mark until the runner presses; a
 * press snaps to the mark. A mark that is passed by more than the release
 * fraction of the segment is taken as a forgotten press: a lap moves on to the
 * next mark, a rep releases the distance.
 */
class TrackDistance {
public:
    struct Segment {
        enum class Kind { Lap, Rep };

        float lengthM;
        Kind  kind;
    };

    explicit TrackDistance(float releaseFraction) : mReleaseFraction(releaseFraction) {}

    void beginSegment(std::optional<Segment> segment) { mSegment = segment; }

    float total(float rawTotalM, float k) const
    {
        const float estimated = estimate(rawTotalM, k);
        const auto  mark      = markM(rawTotalM, k);
        return mark ? std::min(estimated, *mark) : estimated;
    }

    /// Distance of the mark the segment is heading for.
    std::optional<float> markM(float rawTotalM, float k) const
    {
        if (!mSegment) {
            return std::nullopt;
        }
        const float length   = mSegment->lengthM;
        const float released = estimate(rawTotalM, k) - mStartM - mReleaseFraction * length;
        if (mSegment->kind == Segment::Kind::Rep) {
            return released > length ? std::nullopt : std::optional<float>(mStartM + length);
        }
        return mStartM + length * std::max(1.0f, std::ceil(released / length));
    }

    /// Estimated distance left to the mark.
    std::optional<float> toMarkM(float rawTotalM, float k) const
    {
        const auto mark = markM(rawTotalM, k);
        return mark ? std::optional<float>(*mark - estimate(rawTotalM, k)) : std::nullopt;
    }

    /// True when the press comes before the given fraction of the way to its mark.
    bool isEarlyPress(float rawTotalM, float k, float earlyFraction) const
    {
        const auto left = toMarkM(rawTotalM, k);
        return left && *left > (1.0f - earlyFraction) * mSegment->lengthM;
    }

    /// Ends the segment at its mark. Returns the length it covered when that is known.
    std::optional<float> closeSegment(float rawTotalM, float k)
    {
        const auto  mark    = markM(rawTotalM, k);
        const auto  covered = mark ? std::optional<float>(*mark - mStartM) : std::nullopt;
        mStartM    = mark.value_or(estimate(rawTotalM, k));
        mRawStartM = rawTotalM;
        mSegment   = std::nullopt;
        return covered;
    }

    float rawSegmentM(float rawTotalM) const { return rawTotalM - mRawStartM; }

private:
    float estimate(float rawTotalM, float k) const { return mStartM + k * rawSegmentM(rawTotalM); }

    float                  mReleaseFraction;
    std::optional<Segment> mSegment;
    float                  mStartM    = 0.0f;
    float                  mRawStartM = 0.0f;
};

#endif // TRACK_DISTANCE_HPP
