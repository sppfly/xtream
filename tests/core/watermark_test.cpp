#include "core/watermark.h"

#include <gtest/gtest.h>

namespace xtream {
namespace {

TEST(WatermarkTest, ConstructWithTimestampAndSourceId) {
    Watermark wm(u64(1000), u64(3));
    EXPECT_EQ(wm.timestamp(), 1000u);
    EXPECT_EQ(wm.source_id(), 3u);
}

TEST(WatermarkTest, Equality) {
    Watermark w1(u64(100), u64(1));
    Watermark w2(u64(100), u64(1));
    Watermark w3(u64(100), u64(2));
    Watermark w4(u64(200), u64(1));

    EXPECT_EQ(w1, w2);
    EXPECT_NE(w1, w3);
    EXPECT_NE(w1, w4);
}

TEST(WatermarkTest, OrderingByTimestamp) {
    Watermark w1(u64(100), u64(0));
    Watermark w2(u64(200), u64(0));

    EXPECT_LT(w1, w2);
    EXPECT_GT(w2, w1);
    EXPECT_LE(w1, w1);
    EXPECT_GE(w1, w1);
}

TEST(WatermarkTest, OrderingBySourceIdWhenSameTimestamp) {
    Watermark w1(u64(100), u64(1));
    Watermark w2(u64(100), u64(2));

    EXPECT_LT(w1, w2);
}

TEST(WatermarkTest, MinTimestamp) {
    u64 a(100);
    u64 b(200);
    EXPECT_EQ(Watermark::min_timestamp(a, b), 100u);
    EXPECT_EQ(Watermark::min_timestamp(b, a), 100u);
}

TEST(WatermarkTest, MinTimestampEqual) {
    u64 a(100);
    u64 b(100);
    EXPECT_EQ(Watermark::min_timestamp(a, b), 100u);
}

TEST(WatermarkTest, Copy) {
    Watermark wm(u64(500), u64(7));
    Watermark copy = wm;
    EXPECT_EQ(copy, wm);
}

}  // namespace
}  // namespace xtream
