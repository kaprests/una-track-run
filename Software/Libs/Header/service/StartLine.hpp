#ifndef START_LINE_HPP
#define START_LINE_HPP

#include <cmath>
#include <optional>

/**
 * The start line of an outdoor track as a GPS point. A lap is counted at the
 * closest approach of a pass within the capture radius, while the caller
 * reports the lap as far enough along to be armed.
 */
class StartLine {
public:
    static constexpr float kCaptureRadiusM = 12.0f;

    void set(double latDeg, double lonDeg) { mLine = Point{ latDeg, lonDeg }; mClosestM.reset(); }
    void clear()                           { mLine.reset(); mClosestM.reset(); }
    bool isSet() const                     { return mLine.has_value(); }

    /// Feeds one fix. Returns true once per pass, at its closest approach.
    bool update(double latDeg, double lonDeg, bool armed)
    {
        if (!mLine || !armed) {
            mClosestM.reset();
            return false;
        }
        const float distanceM = distanceToLineM(latDeg, lonDeg);
        const bool  inside    = distanceM <= kCaptureRadiusM;
        if (inside && (!mClosestM || distanceM <= *mClosestM)) {
            mClosestM = distanceM;
            return false;
        }
        const bool passed = mClosestM.has_value();
        mClosestM.reset();
        return passed;
    }

    float distanceToLineM(double latDeg, double lonDeg) const
    {
        constexpr double kMetresPerDegree = 111320.0;
        constexpr double kRadPerDeg       = 3.14159265358979323846 / 180.0;
        const double north = (latDeg - mLine->latDeg) * kMetresPerDegree;
        const double east  = (lonDeg - mLine->lonDeg) * kMetresPerDegree * std::cos(mLine->latDeg * kRadPerDeg);
        return static_cast<float>(std::hypot(north, east));
    }

private:
    struct Point {
        double latDeg;
        double lonDeg;
    };

    std::optional<Point> mLine;
    std::optional<float> mClosestM;
};

#endif // START_LINE_HPP
