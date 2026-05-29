#include "core/types/int.h"

#include <gtest/gtest.h>

#include "core/types/uint.h"

namespace extream {
namespace {

TEST(I32Test, Construct) {
    i32 a(42);
    EXPECT_EQ(a.raw(), 42);
}

TEST(I32Test, NoImplicitFromRaw) {
    static_assert(!std::is_convertible_v<int32_t, i32>);
    static_assert(!std::is_convertible_v<i32, int32_t>);
}

TEST(I32Test, NoImplicitCrossType) {
    u32 u(10);
    static_assert(!std::is_convertible_v<u32, i32>);
    static_assert(!std::is_convertible_v<i32, u32>);
    static_assert(!std::is_constructible_v<i32, u32>);
}

TEST(I32Test, Add) {
    i32 a(10);
    i32 b(20);
    EXPECT_EQ((a + b).raw(), 30);
}

TEST(I32Test, Sub) {
    i32 a(30);
    i32 b(10);
    EXPECT_EQ((a - b).raw(), 20);
}

TEST(I32Test, Mul) {
    i32 a(5);
    i32 b(6);
    EXPECT_EQ((a * b).raw(), 30);
}

TEST(I32Test, Div) {
    i32 a(30);
    i32 b(5);
    EXPECT_EQ((a / b).raw(), 6);
}

TEST(I32Test, Mod) {
    i32 a(30);
    i32 b(7);
    EXPECT_EQ((a % b).raw(), 2);
}

TEST(I32Test, Negate) {
    i32 a(42);
    EXPECT_EQ((-a).raw(), -42);
}

TEST(I32Test, AddAssign) {
    i32 a(10);
    a += i32(5);
    EXPECT_EQ(a.raw(), 15);
}

TEST(I32Test, SubAssign) {
    i32 a(10);
    a -= i32(3);
    EXPECT_EQ(a.raw(), 7);
}

TEST(I32Test, MulAssign) {
    i32 a(5);
    a *= i32(3);
    EXPECT_EQ(a.raw(), 15);
}

TEST(I32Test, DivAssign) {
    i32 a(20);
    a /= i32(4);
    EXPECT_EQ(a.raw(), 5);
}

TEST(I32Test, ModAssign) {
    i32 a(20);
    a %= i32(7);
    EXPECT_EQ(a.raw(), 6);
}

TEST(I32Test, PreIncrement) {
    i32 a(10);
    i32& ref = ++a;
    EXPECT_EQ(a.raw(), 11);
    EXPECT_EQ(ref.raw(), 11);
}

TEST(I32Test, PostIncrement) {
    i32 a(10);
    i32 old = a++;
    EXPECT_EQ(old.raw(), 10);
    EXPECT_EQ(a.raw(), 11);
}

TEST(I32Test, PreDecrement) {
    i32 a(10);
    --a;
    EXPECT_EQ(a.raw(), 9);
}

TEST(I32Test, PostDecrement) {
    i32 a(10);
    i32 old = a--;
    EXPECT_EQ(old.raw(), 10);
    EXPECT_EQ(a.raw(), 9);
}

TEST(I32Test, BitAnd) {
    i32 a(0b1100);
    i32 b(0b1010);
    EXPECT_EQ((a & b).raw(), 0b1000);
}

TEST(I32Test, BitOr) {
    i32 a(0b1100);
    i32 b(0b1010);
    EXPECT_EQ((a | b).raw(), 0b1110);
}

TEST(I32Test, BitXor) {
    i32 a(0b1100);
    i32 b(0b1010);
    EXPECT_EQ((a ^ b).raw(), 0b0110);
}

TEST(I32Test, BitNot) {
    i32 a(0);
    EXPECT_EQ((~a).raw(), -1);
}

TEST(I32Test, LeftShift) {
    i32 a(1);
    EXPECT_EQ((a << i32(3)).raw(), 8);
}

TEST(I32Test, RightShift) {
    i32 a(16);
    EXPECT_EQ((a >> i32(2)).raw(), 4);
}

TEST(I32Test, CompareEqual) {
    EXPECT_EQ(i32(5), i32(5));
    EXPECT_NE(i32(5), i32(6));
}

TEST(I32Test, CompareLess) {
    EXPECT_LT(i32(5), i32(10));
    EXPECT_LE(i32(5), i32(5));
}

TEST(I32Test, CompareGreater) {
    EXPECT_GT(i32(10), i32(5));
    EXPECT_GE(i32(5), i32(5));
}

TEST(I32Test, AsConversion) {
    i32 a(42);
    u32 b = a.as<uint32_t>();
    EXPECT_EQ(b.raw(), 42u);
}

TEST(I32Test, DivByZeroAsserts) {
    i32 a(10);
    i32 b(0);
    EXPECT_DEATH(a / b, "");
}

TEST(I32Test, NegMinAsserts) {
    i32 a(std::numeric_limits<int32_t>::min());
    EXPECT_DEATH(-a, "");
}

TEST(I8Test, ConstructAndWrap) {
    i8 a(127);
    EXPECT_EQ(a.raw(), 127);
}

TEST(I8Test, LeftShiftPromotion) {
    i8 a(1);
    EXPECT_EQ((a << i8(3)).raw(), 8);
}

TEST(I16Test, Construct) {
    i16 a(1000);
    EXPECT_EQ(a.raw(), 1000);
}

TEST(I64Test, Construct) {
    i64 a(10000000000LL);
    EXPECT_EQ(a.raw(), 10000000000LL);
}

TEST(I32Test, Constexpr) {
    constexpr i32 a(10);
    constexpr i32 b(20);
    constexpr i32 c = a + b;
    static_assert(c.raw() == 30);
}

}  // namespace
}  // namespace extream
