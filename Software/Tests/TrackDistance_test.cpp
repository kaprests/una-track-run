#include <gtest/gtest.h>

#include "TrackDistance.hpp"

TEST(TrackDistance, ScalesModelDistanceByK)
{
    TrackDistance distance;
    EXPECT_FLOAT_EQ(distance.total(100.0f, 1.1f), 110.0f);
}

TEST(TrackDistance, KnownSegmentFixesDistance)
{
    TrackDistance distance;
    distance.closeSegment(190.0f, 1.0f, 200.0f);
    EXPECT_FLOAT_EQ(distance.total(190.0f, 1.0f), 200.0f);
    EXPECT_FLOAT_EQ(distance.total(290.0f, 1.0f), 300.0f);
}

TEST(TrackDistance, SegmentWithoutKnownLengthKeepsEstimate)
{
    TrackDistance distance;
    distance.closeSegment(100.0f, 1.1f, std::nullopt);
    EXPECT_FLOAT_EQ(distance.total(100.0f, 1.1f), 110.0f);
    EXPECT_FLOAT_EQ(distance.total(200.0f, 1.1f), 220.0f);
}

TEST(TrackDistance, NeverDecreases)
{
    TrackDistance distance;
    distance.total(210.0f, 1.0f);
    distance.closeSegment(210.0f, 1.0f, 200.0f);
    EXPECT_FLOAT_EQ(distance.total(210.0f, 1.0f), 210.0f);
    EXPECT_FLOAT_EQ(distance.total(215.0f, 1.0f), 210.0f);
    EXPECT_FLOAT_EQ(distance.total(230.0f, 1.0f), 220.0f);
}

TEST(TrackDistance, ReportsModelDistanceOfCurrentSegment)
{
    TrackDistance distance;
    distance.closeSegment(190.0f, 1.0f, 200.0f);
    EXPECT_FLOAT_EQ(distance.rawSegmentM(250.0f), 60.0f);
}
