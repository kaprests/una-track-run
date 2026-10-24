#include <gtest/gtest.h>

#include "SegmentCorrector.hpp"

namespace {

constexpr float kLapM       = 200.0f;
constexpr float kLapSeconds = 50.0f;

float runLaps(SegmentCorrector& corrector, float trueFactor, int laps)
{
    for (int i = 0; i < laps; ++i) {
        corrector.anchor(kLapM / trueFactor, kLapM, kLapSeconds);
    }
    return corrector.k();
}

} // namespace

TEST(SegmentCorrector, ConvergesFromBiasedModel)
{
    SegmentCorrector corrector;
    EXPECT_NEAR(runLaps(corrector, 1.05f, 6), 1.05f, 0.005f);
}

TEST(SegmentCorrector, FirstLapsMoveMostOfTheWay)
{
    SegmentCorrector corrector;
    runLaps(corrector, 1.05f, 2);
    EXPECT_GT(corrector.k(), 1.03f);
}

TEST(SegmentCorrector, RejectsMissedLapPress)
{
    SegmentCorrector corrector;
    runLaps(corrector, 1.0f, 5);
    EXPECT_FALSE(corrector.anchor(2.0f * kLapM, kLapM, 2.0f * kLapSeconds));
    EXPECT_FLOAT_EQ(corrector.k(), 1.0f);
}

TEST(SegmentCorrector, RejectsWalkingSegment)
{
    SegmentCorrector corrector;
    EXPECT_FALSE(corrector.anchor(100.0f, kLapM, 120.0f));
}

TEST(SegmentCorrector, RejectsImplausibleFactor)
{
    SegmentCorrector corrector;
    EXPECT_FALSE(corrector.anchor(100.0f, kLapM, 40.0f));
}

TEST(SegmentCorrector, FollowsFatigueDrift)
{
    SegmentCorrector corrector;
    runLaps(corrector, 1.0f, 8);
    for (int lap = 0; lap < 30; ++lap) {
        corrector.drift(kLapSeconds);
        corrector.anchor(kLapM / (1.0f + 0.001f * static_cast<float>(lap)), kLapM, kLapSeconds);
    }
    EXPECT_NEAR(corrector.k(), 1.029f, 0.007f);
}

TEST(SegmentCorrector, LongerSegmentsCountForMore)
{
    SegmentCorrector shortRep;
    SegmentCorrector longRep;
    shortRep.anchor(100.0f / 1.04f, 100.0f, 25.0f);
    longRep.anchor(1000.0f / 1.04f, 1000.0f, 250.0f);
    EXPECT_GT(longRep.k(), shortRep.k());
}

TEST(SegmentCorrector, RepsOfThreeHundredMetres)
{
    SegmentCorrector corrector;
    for (int i = 0; i < 5; ++i) {
        corrector.anchor(300.0f / 1.03f, 300.0f, 60.0f);
    }
    EXPECT_NEAR(corrector.k(), 1.03f, 0.004f);
}
