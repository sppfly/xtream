#include "core/types/float.h"

#include <gtest/gtest.h>

namespace xtream {
namespace {

TEST(F32Test, Construct) {
    f32 a(3.14f);
    EXPECT_FLOAT_EQ(a.raw(), 3.14f);
}

TEST(F32Test, NoImplicitFromDouble) {
    static_assert(!std::is_constructible_v<f32, double>);
}

TEST(F32Test, NoImplicitFromRaw) {
    static_assert(!std::is_convertible_v<float, f32>);
}

TEST(F32Test, Add) {
    f32 a(1.5f);
    f32 b(2.5f);
    EXPECT_FLOAT_EQ((a + b).raw(), 4.0f);
}

TEST(F32Test, Sub) {
    f32 a(5.0f);
    f32 b(2.0f);
    EXPECT_FLOAT_EQ((a - b).raw(), 3.0f);
}

TEST(F32Test, Mul) {
    f32 a(3.0f);
    f32 b(4.0f);
    EXPECT_FLOAT_EQ((a * b).raw(), 12.0f);
}

TEST(F32Test, Div) {
    f32 a(12.0f);
    f32 b(3.0f);
    EXPECT_FLOAT_EQ((a / b).raw(), 4.0f);
}

TEST(F32Test, Negate) {
    f32 a(5.0f);
    EXPECT_FLOAT_EQ((-a).raw(), -5.0f);
}

TEST(F32Test, AddAssign) {
    f32 a(1.0f);
    a += f32(2.0f);
    EXPECT_FLOAT_EQ(a.raw(), 3.0f);
}

TEST(F32Test, CompareEqual) {
    EXPECT_EQ(f32(1.0f), f32(1.0f));
    EXPECT_NE(f32(1.0f), f32(2.0f));
}

TEST(F32Test, CompareLess) {
    EXPECT_LT(f32(1.0f), f32(2.0f));
}

TEST(F32Test, CompareGreater) {
    EXPECT_GT(f32(2.0f), f32(1.0f));
}

TEST(F64Test, Construct) {
    f64 a(3.14159265358979);
    EXPECT_DOUBLE_EQ(a.raw(), 3.14159265358979);
}

TEST(F64Test, NoImplicitFromFloat) {
    static_assert(!std::is_constructible_v<f64, float>);
}

TEST(F64Test, Arithmetic) {
    f64 a(10.0);
    f64 b(3.0);
    EXPECT_DOUBLE_EQ((a + b).raw(), 13.0);
    EXPECT_DOUBLE_EQ((a - b).raw(), 7.0);
    EXPECT_DOUBLE_EQ((a * b).raw(), 30.0);
    EXPECT_DOUBLE_EQ((a / b).raw(), 10.0 / 3.0);
}

TEST(F32Test, Constexpr) {
    constexpr f32 a(1.5f);
    constexpr f32 b(2.5f);
    constexpr f32 c = a + b;
    static_assert(c.raw() == 4.0f);
}

}  // namespace
}  // namespace xtream
