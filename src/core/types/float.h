#pragma once

#include <compare>
#include <format>

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

constexpr f32 operator""_f32(long double v) {
    return f32(static_cast<float>(v));
}
constexpr f64 operator""_f64(long double v) {
    return f64(static_cast<double>(v));
}

}  // namespace xtream

template <>
struct std::formatter<xtream::f32> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    auto format(const xtream::f32& v, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{}", v.raw());
    }
};

template <>
struct std::formatter<xtream::f64> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    auto format(const xtream::f64& v, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{}", v.raw());
    }
};
