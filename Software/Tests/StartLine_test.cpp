#include <gtest/gtest.h>

#include <cmath>
#include <vector>

#include "StartLine.hpp"
#include "TrackDistance.hpp"

namespace {

constexpr double kLat0           = 59.9;
constexpr double kLon0           = 10.7;
constexpr double kMetresPerDeg   = 111320.0;
constexpr double kStraightM      = 84.39;
constexpr double kRadiusM        = 36.5;
constexpr double kPi             = 3.14159265358979323846;
constexpr double kLapM           = 2.0 * kStraightM + 2.0 * kPi * kRadiusM;

struct Fix {
    double lat;
    double lon;
};

/// Position on a 400 m track, `s` metres from the line at the start of the home straight.
Fix onTrack(double s, double noiseM = 0.0)
{
    s = std::fmod(s, kLapM);
    const double bend = kPi * kRadiusM;
    double x;
    double y;
    if (s < kStraightM) {
        x = s;
        y = -kRadiusM;
    } else if (s < kStraightM + bend) {
        const double a = (s - kStraightM) / kRadiusM;
        x = kStraightM + kRadiusM * std::sin(a);
        y = -kRadiusM * std::cos(a);
    } else if (s < 2.0 * kStraightM + bend) {
        x = kStraightM - (s - kStraightM - bend);
        y = kRadiusM;
    } else {
        const double a = (s - 2.0 * kStraightM - bend) / kRadiusM;
        x = -kRadiusM * std::sin(a);
        y = kRadiusM * std::cos(a);
    }
    x += noiseM;
    y += kRadiusM + noiseM;
    return { kLat0 + y / kMetresPerDeg, kLon0 + x / (kMetresPerDeg * std::cos(kLat0 * kPi / 180.0)) };
}

/// Runs `metres` at 4 m/s with 1 Hz fixes and returns the distances at which laps were counted.
std::vector<double> run(double metres, double jitterM = 0.0)
{
    StartLine     line;
    TrackDistance distance{ 0.2f };
    distance.beginSegment(TrackDistance::Segment{ static_cast<float>(kLapM), TrackDistance::Segment::Kind::Lap });
    line.set(onTrack(0.0).lat, onTrack(0.0).lon);

    std::vector<double> laps;
    for (double s = 4.0; s <= metres; s += 4.0) {
        const double noise = (static_cast<int>(s) % 8 == 0 ? 1.0 : -1.0) * jitterM;
        const Fix    fix   = onTrack(s, noise);
        const bool   armed = !distance.isEarlyPress(static_cast<float>(s), 1.0f, 0.8f);
        if (line.update(fix.lat, fix.lon, armed)) {
            laps.push_back(s);
            distance.closeSegment(static_cast<float>(s), 1.0f);
            distance.beginSegment(TrackDistance::Segment{ static_cast<float>(kLapM), TrackDistance::Segment::Kind::Lap });
        }
    }
    return laps;
}

} // namespace

TEST(StartLine, MeasuresDistanceToTheLine)
{
    StartLine line;
    line.set(kLat0, kLon0);
    EXPECT_NEAR(line.distanceToLineM(kLat0 + 10.0 / kMetresPerDeg, kLon0), 10.0f, 0.01f);
}

TEST(StartLine, CountsOneLapPerPassAtTheClosestApproach)
{
    const std::vector<double> laps = run(3.0 * kLapM + 20.0);
    ASSERT_EQ(laps.size(), 3u);
    for (size_t i = 0; i < laps.size(); ++i) {
        EXPECT_NEAR(laps[i], static_cast<double>(i + 1) * kLapM, 6.0);
    }
}

TEST(StartLine, GpsJitterAtTheLineCountsOnce)
{
    EXPECT_EQ(run(kLapM + 30.0, 3.0).size(), 1u);
}

TEST(StartLine, NoLapWhileNotArmed)
{
    StartLine line;
    line.set(kLat0, kLon0);
    EXPECT_FALSE(line.update(kLat0, kLon0, false));
    EXPECT_FALSE(line.update(kLat0 + 20.0 / kMetresPerDeg, kLon0, false));
}

TEST(StartLine, NoLapUntilALineIsSet)
{
    StartLine line;
    EXPECT_FALSE(line.update(kLat0, kLon0, true));
    EXPECT_FALSE(line.update(kLat0 + 20.0 / kMetresPerDeg, kLon0, true));
}

TEST(StartLine, BackStraightDoesNotPassTheLine)
{
    StartLine line;
    line.set(onTrack(0.0).lat, onTrack(0.0).lon);
    for (double s = 2.0 * StartLine::kCaptureRadiusM; s < kLapM - 20.0; s += 4.0) {
        const Fix fix = onTrack(s);
        EXPECT_FALSE(line.update(fix.lat, fix.lon, true));
    }
}
