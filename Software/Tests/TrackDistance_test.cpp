#include <gtest/gtest.h>

#include "SDK/Metrics/MonotonicCounter.hpp"
#include "TrackDistance.hpp"

namespace {

constexpr float kLapM = 200.0f;

TrackDistance trackWithLaps()
{
    TrackDistance distance;
    distance.beginSegment(TrackDistance::Segment{ kLapM, true });
    return distance;
}

} // namespace

TEST(TrackDistance, FollowsEstimateBeforeTheMark)
{
    const TrackDistance distance = trackWithLaps();
    EXPECT_FLOAT_EQ(distance.total(150.0f, 1.0f), 150.0f);
}

TEST(TrackDistance, HoldsAtTheMarkWhenModelOvershoots)
{
    const TrackDistance distance = trackWithLaps();
    EXPECT_FLOAT_EQ(distance.total(215.0f, 1.0f), 200.0f);
}

TEST(TrackDistance, PressSnapsToTheMarkFromBelow)
{
    TrackDistance distance = trackWithLaps();
    EXPECT_EQ(distance.closeSegment(190.0f, 1.0f), std::optional<float>(200.0f));
    distance.beginSegment(TrackDistance::Segment{ kLapM, true });
    EXPECT_FLOAT_EQ(distance.total(190.0f, 1.0f), 200.0f);
}

TEST(TrackDistance, PressSnapsToTheMarkFromAbove)
{
    TrackDistance distance = trackWithLaps();
    EXPECT_EQ(distance.closeSegment(215.0f, 1.0f), std::optional<float>(200.0f));
    distance.beginSegment(TrackDistance::Segment{ kLapM, true });
    EXPECT_FLOAT_EQ(distance.total(215.0f, 1.0f), 200.0f);
    EXPECT_FLOAT_EQ(distance.total(265.0f, 1.0f), 250.0f);
}

TEST(TrackDistance, ForgottenPressReleasesAndHoldsAtTheNextMark)
{
    const TrackDistance distance = trackWithLaps();
    EXPECT_FLOAT_EQ(distance.total(240.0f, 1.0f), 200.0f);
    EXPECT_FLOAT_EQ(distance.total(241.0f, 1.0f), 241.0f);
    EXPECT_FLOAT_EQ(distance.total(390.0f, 1.0f), 390.0f);
    EXPECT_FLOAT_EQ(distance.total(420.0f, 1.0f), 400.0f);
}

TEST(TrackDistance, PressAfterForgottenLapCoversBothLaps)
{
    TrackDistance distance = trackWithLaps();
    EXPECT_EQ(distance.closeSegment(395.0f, 1.0f), std::optional<float>(400.0f));
}

TEST(TrackDistance, TenLapsLandExactlyOnTheMarks)
{
    TrackDistance distance = trackWithLaps();
    float raw = 0.0f;
    for (int lap = 1; lap <= 10; ++lap) {
        raw += (lap % 2 == 0) ? 192.0f : 207.0f;
        distance.closeSegment(raw, 1.0f);
        distance.beginSegment(TrackDistance::Segment{ kLapM, true });
        EXPECT_FLOAT_EQ(distance.total(raw, 1.0f), static_cast<float>(lap) * kLapM);
    }
}

TEST(TrackDistance, NeverDecreases)
{
    TrackDistance distance = trackWithLaps();
    float previous = 0.0f;
    float raw      = 0.0f;
    for (int step = 0; step < 150; ++step) {
        raw += 4.0f;
        const float shown = distance.total(raw, 1.0f);
        EXPECT_GE(shown, previous);
        previous = shown;
        if (step % 50 == 49) {
            distance.closeSegment(raw, 1.0f);
            distance.beginSegment(TrackDistance::Segment{ kLapM, true });
        }
    }
}

TEST(TrackDistance, ScalesModelDistanceByK)
{
    TrackDistance distance;
    EXPECT_FLOAT_EQ(distance.total(100.0f, 1.1f), 110.0f);
}

TEST(TrackDistance, SegmentWithoutLengthIsNotCappedAndNotAnchored)
{
    TrackDistance distance;
    EXPECT_FLOAT_EQ(distance.total(500.0f, 1.0f), 500.0f);
    EXPECT_EQ(distance.closeSegment(500.0f, 1.1f), std::nullopt);
    EXPECT_FLOAT_EQ(distance.total(600.0f, 1.1f), 550.0f + 110.0f);
}

TEST(TrackDistance, RepEndsOnOnePressAndReleasesWhenForgotten)
{
    TrackDistance distance;
    distance.beginSegment(TrackDistance::Segment{ 300.0f, false });
    EXPECT_FLOAT_EQ(distance.total(320.0f, 1.0f), 300.0f);
    EXPECT_FLOAT_EQ(distance.total(361.0f, 1.0f), 361.0f);
    EXPECT_FLOAT_EQ(distance.total(700.0f, 1.0f), 700.0f);
    EXPECT_EQ(distance.closeSegment(700.0f, 1.0f), std::nullopt);
}

TEST(TrackDistance, EarlyPressIsOnlyBeforeTheFractionOfTheLap)
{
    const TrackDistance distance = trackWithLaps();
    EXPECT_TRUE(distance.isEarlyPress(90.0f, 1.0f, 0.5f));
    EXPECT_FALSE(distance.isEarlyPress(110.0f, 1.0f, 0.5f));
}

TEST(TrackDistance, EarlyPressIsJudgedWithinTheCurrentLapAfterAForgottenPress)
{
    const TrackDistance distance = trackWithLaps();
    EXPECT_TRUE(distance.isEarlyPress(260.0f, 1.0f, 0.5f));
    EXPECT_FALSE(distance.isEarlyPress(310.0f, 1.0f, 0.5f));
}

TEST(TrackDistance, SegmentWithoutLengthHasNoEarlyPress)
{
    const TrackDistance distance;
    EXPECT_FALSE(distance.isEarlyPress(5.0f, 1.0f, 0.5f));
}

TEST(TrackDistance, ReportsModelDistanceOfCurrentSegment)
{
    TrackDistance distance = trackWithLaps();
    distance.closeSegment(190.0f, 1.0f);
    EXPECT_FLOAT_EQ(distance.rawSegmentM(250.0f), 60.0f);
}

TEST(TrackDistance, SessionTotalEqualsSumOfLapsInTheCounter)
{
    SDK::Metric::MonotonicCounter<float> counter;
    counter.init();
    counter.add(0.0f);

    TrackDistance distance = trackWithLaps();
    float raw     = 0.0f;
    float lapsSum = 0.0f;
    const auto press = [&] {
        distance.closeSegment(raw, 1.0f);
        distance.beginSegment(TrackDistance::Segment{ kLapM, true });
        counter.add(distance.total(raw, 1.0f));
        lapsSum += counter.getLapValueActive();
        counter.resetLap();
    };

    for (const float lapRaw : { 207.0f, 193.0f, 401.0f, 196.0f }) {
        for (float step = 3.0f; step <= lapRaw; step += 3.0f) {
            counter.add(distance.total(raw + step, 1.0f));
        }
        raw += lapRaw;
        counter.add(distance.total(raw, 1.0f));
        press();
    }

    EXPECT_FLOAT_EQ(counter.getValueActive(), 1000.0f);
    EXPECT_FLOAT_EQ(lapsSum, 1000.0f);
}
