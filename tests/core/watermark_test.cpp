#include "core/watermark.h"

#include <gtest/gtest.h>

namespace extream {
namespace {

TEST(WatermarkTest, ConstructWithTimestampAndSourceId) {
    Watermark wm(i64(1000), u64(3));
    EXPECT_EQ(wm.timestamp().raw(), 1000);
    EXPECT_EQ(wm.source_id().raw(), 3u);
}

TEST(WatermarkTest, Equality) {
    Watermark w1(i64(100), u64(1));
    Watermark w2(i64(100), u64(1));
    Watermark w3(i64(100), u64(2));
    Watermark w4(i64(200), u64(1));

    EXPECT_EQ(w1, w2);
    EXPECT_NE(w1, w3);
    EXPECT_NE(w1, w4);
}

TEST(WatermarkTest, OrderingByTimestamp) {
    Watermark w1(i64(100), u64(0));
    Watermark w2(i64(200), u64(0));

    EXPECT_LT(w1, w2);
    EXPECT_GT(w2, w1);
    EXPECT_LE(w1, w1);
    EXPECT_GE(w1, w1);
}

TEST(WatermarkTest, OrderingBySourceIdWhenSameTimestamp) {
    Watermark w1(i64(100), u64(1));
    Watermark w2(i64(100), u64(2));

    EXPECT_LT(w1, w2);
}

TEST(WatermarkTest, MinTimestamp) {
    i64 a(100);
    i64 b(200);
    EXPECT_EQ(Watermark::min_timestamp(a, b).raw(), 100);
    EXPECT_EQ(Watermark::min_timestamp(b, a).raw(), 100);
}

TEST(WatermarkTest, MinTimestampEqual) {
    i64 a(100);
    i64 b(100);
    EXPECT_EQ(Watermark::min_timestamp(a, b).raw(), 100);
}

TEST(WatermarkTest, Copy) {
    Watermark wm(i64(500), u64(7));
    Watermark copy = wm;
    EXPECT_EQ(copy, wm);
}

}  // namespace
}  // namespace extream
