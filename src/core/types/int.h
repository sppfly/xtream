#pragma once

#include <cassert>
#include <compare>
#include <concepts>
#include <cstdint>
#include <format>
#include <limits>
#include <type_traits>

namespace xtream {

template <std::integral T>
class StrictInt {
public:
    using underlying_type = T;

    constexpr explicit StrictInt(T v) : value_(v) {}

    template <std::integral U>
    explicit StrictInt(StrictInt<U> other) = delete;

    // Deprecated
    [[nodiscard]] constexpr T raw() const { return value_; }

    constexpr operator T() const { return value_; }

    template <std::integral U>
    constexpr StrictInt<U> as() const {
        return StrictInt<U>(static_cast<U>(value_));
    }

    constexpr StrictInt operator+(StrictInt other) const {
        T result;
        bool overflow = __builtin_add_overflow(value_, other.value_, &result);
        if constexpr (std::is_signed_v<T>) assert(!overflow);
        return StrictInt(result);
    }

    constexpr StrictInt operator-(StrictInt other) const {
        T result;
        bool overflow = __builtin_sub_overflow(value_, other.value_, &result);
        if constexpr (std::is_signed_v<T>) assert(!overflow);
        return StrictInt(result);
    }

    constexpr StrictInt operator*(StrictInt other) const {
        T result;
        bool overflow = __builtin_mul_overflow(value_, other.value_, &result);
        if constexpr (std::is_signed_v<T>) assert(!overflow);
        return StrictInt(result);
    }

    constexpr StrictInt operator/(StrictInt other) const {
        assert(other.value_ != T(0));
        if constexpr (std::is_signed_v<T>) {
            assert(!(value_ == std::numeric_limits<T>::min() && other.value_ == T(-1)));
        }
        return StrictInt(value_ / other.value_);
    }

    constexpr StrictInt operator%(StrictInt other) const {
        assert(other.value_ != T(0));
        return StrictInt(value_ % other.value_);
    }

    constexpr StrictInt operator-() const {
        T result;
        bool overflow = __builtin_sub_overflow(T(0), value_, &result);
        if constexpr (std::is_signed_v<T>) assert(!overflow);
        return StrictInt(result);
    }

    constexpr StrictInt operator&(StrictInt other) const {
        return StrictInt(value_ & other.value_);
    }
    constexpr StrictInt operator|(StrictInt other) const {
        return StrictInt(value_ | other.value_);
    }
    constexpr StrictInt operator^(StrictInt other) const {
        return StrictInt(value_ ^ other.value_);
    }
    constexpr StrictInt operator~() const { return StrictInt(~value_); }

    constexpr StrictInt operator<<(StrictInt other) const {
        return StrictInt(static_cast<T>(value_ << other.value_));
    }
    constexpr StrictInt operator>>(StrictInt other) const {
        return StrictInt(static_cast<T>(value_ >> other.value_));
    }

    constexpr StrictInt& operator+=(StrictInt other) {
        *this = *this + other;
        return *this;
    }
    constexpr StrictInt& operator-=(StrictInt other) {
        *this = *this - other;
        return *this;
    }
    constexpr StrictInt& operator*=(StrictInt other) {
        *this = *this * other;
        return *this;
    }
    constexpr StrictInt& operator/=(StrictInt other) {
        *this = *this / other;
        return *this;
    }
    constexpr StrictInt& operator%=(StrictInt other) {
        *this = *this % other;
        return *this;
    }
    constexpr StrictInt& operator&=(StrictInt other) {
        *this = *this & other;
        return *this;
    }
    constexpr StrictInt& operator|=(StrictInt other) {
        *this = *this | other;
        return *this;
    }
    constexpr StrictInt& operator^=(StrictInt other) {
        *this = *this ^ other;
        return *this;
    }
    constexpr StrictInt& operator<<=(StrictInt other) {
        *this = *this << other;
        return *this;
    }
    constexpr StrictInt& operator>>=(StrictInt other) {
        *this = *this >> other;
        return *this;
    }

    constexpr StrictInt& operator++() {
        *this = *this + StrictInt(T(1));
        return *this;
    }
    constexpr StrictInt operator++(int) {
        StrictInt old = *this;
        ++(*this);
        return old;
    }
    constexpr StrictInt& operator--() {
        *this = *this - StrictInt(T(1));
        return *this;
    }
    constexpr StrictInt operator--(int) {
        StrictInt old = *this;
        --(*this);
        return old;
    }

    constexpr auto operator<=>(const StrictInt&) const = default;

private:
    T value_;
};

using i8 = StrictInt<int8_t>;
using i16 = StrictInt<int16_t>;
using i32 = StrictInt<int32_t>;
using i64 = StrictInt<int64_t>;

constexpr i8 operator""_i8(unsigned long long v) {
    return i8(static_cast<int8_t>(v));
}
constexpr i16 operator""_i16(unsigned long long v) {
    return i16(static_cast<int16_t>(v));
}
constexpr i32 operator""_i32(unsigned long long v) {
    return i32(static_cast<int32_t>(v));
}
constexpr i64 operator""_i64(unsigned long long v) {
    return i64(static_cast<int64_t>(v));
}

}  // namespace xtream

template <std::integral T>
struct std::formatter<xtream::StrictInt<T>> {
    constexpr auto parse(std::format_parse_context& ctx) { return ctx.begin(); }

    auto format(const xtream::StrictInt<T>& v, std::format_context& ctx) const {
        return std::format_to(ctx.out(), "{}", v.raw());
    }
};
