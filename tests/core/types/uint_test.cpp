#include "core/types/uint.h"

#include <gtest/gtest.h>

namespace extream {
namespace {

TEST(U32Test, Construct) {
    u32 a(42);
    EXPECT_EQ(a.raw(), 42u);
}

TEST(U32Test, NoImplicitFromRaw) {
    static_assert(!std::is_convertible_v<uint32_t, u32>);
    static_assert(!std::is_convertible_v<u32, uint32_t>);
}

TEST(U32Test, NoImplicitCrossType) {
    static_assert(!std::is_convertible_v<i32, u32>);
    static_assert(!std::is_convertible_v<u32, i32>);
    static_assert(!std::is_constructible_v<u32, i32>);
}

TEST(U32Test, Add) {
    u32 a(10);
    u32 b(20);
    EXPECT_EQ((a + b).raw(), 30u);
}

TEST(U32Test, Sub) {
    u32 a(30);
    u32 b(10);
    EXPECT_EQ((a - b).raw(), 20u);
}

TEST(U32Test, SubWrapsAround) {
    u32 a(5);
    u32 b(10);
    EXPECT_EQ((a - b).raw(), 4294967291u);
}

TEST(U32Test, Mul) {
    u32 a(5);
    u32 b(6);
    EXPECT_EQ((a * b).raw(), 30u);
}

TEST(U32Test, Div) {
    u32 a(30);
    u32 b(5);
    EXPECT_EQ((a / b).raw(), 6u);
}

TEST(U32Test, Mod) {
    u32 a(30);
    u32 b(7);
    EXPECT_EQ((a % b).raw(), 2u);
}

TEST(U32Test, BitOperations) {
    u32 a(0xF0);
    u32 b(0x0F);
    EXPECT_EQ((a & b).raw(), 0u);
    EXPECT_EQ((a | b).raw(), 0xFFu);
    EXPECT_EQ((a ^ b).raw(), 0xFFu);
    EXPECT_EQ((~u32(0u)).raw(), 0xFFFFFFFFu);
}

TEST(U32Test, Compare) {
    EXPECT_EQ(u32(5), u32(5));
    EXPECT_LT(u32(5), u32(10));
    EXPECT_GT(u32(10), u32(5));
}

TEST(U64Test, Construct) {
    u64 a(10000000000ULL);
    EXPECT_EQ(a.raw(), 10000000000ULL);
}

TEST(U8Test, Construct) {
    u8 a(255);
    EXPECT_EQ(a.raw(), 255u);
}

TEST(U16Test, Construct) {
    u16 a(1000);
    EXPECT_EQ(a.raw(), 1000u);
}

TEST(U32Test, Constexpr) {
    constexpr u32 a(10);
    constexpr u32 b(20);
    constexpr u32 c = a + b;
    static_assert(c.raw() == 30u);
}

}  // namespace
}  // namespace extream
