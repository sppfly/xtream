#pragma once

#include <compare>

namespace xtream {

class f32 {
public:
    constexpr explicit f32(float v) : value_(v) {}
    explicit f32(double) = delete;

    [[nodiscard]] constexpr float raw() const { return value_; }

    constexpr f32 operator+(f32 other) const { return f32(value_ + other.value_); }
    constexpr f32 operator-(f32 other) const { return f32(value_ - other.value_); }
    constexpr f32 operator*(f32 other) const { return f32(value_ * other.value_); }
    constexpr f32 operator/(f32 other) const { return f32(value_ / other.value_); }
    constexpr f32 operator-() const { return f32(-value_); }

    constexpr f32& operator+=(f32 other) {
        *this = *this + other;
        return *this;
    }
    constexpr f32& operator-=(f32 other) {
        *this = *this - other;
        return *this;
    }
    constexpr f32& operator*=(f32 other) {
        *this = *this * other;
        return *this;
    }
    constexpr f32& operator/=(f32 other) {
        *this = *this / other;
        return *this;
    }

    constexpr auto operator<=>(const f32&) const = default;

private:
    float value_;
};

class f64 {
public:
    constexpr explicit f64(double v) : value_(v) {}
    explicit f64(float) = delete;

    [[nodiscard]] constexpr double raw() const { return value_; }

    constexpr f64 operator+(f64 other) const { return f64(value_ + other.value_); }
    constexpr f64 operator-(f64 other) const { return f64(value_ - other.value_); }
    constexpr f64 operator*(f64 other) const { return f64(value_ * other.value_); }
    constexpr f64 operator/(f64 other) const { return f64(value_ / other.value_); }
    constexpr f64 operator-() const { return f64(-value_); }

    constexpr f64& operator+=(f64 other) {
        *this = *this + other;
        return *this;
    }
    constexpr f64& operator-=(f64 other) {
        *this = *this - other;
        return *this;
    }
    constexpr f64& operator*=(f64 other) {
        *this = *this * other;
        return *this;
    }
    constexpr f64& operator/=(f64 other) {
        *this = *this / other;
        return *this;
    }

    constexpr auto operator<=>(const f64&) const = default;

private:
    double value_;
};

}  // namespace xtream
