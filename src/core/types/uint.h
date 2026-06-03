#pragma once

#include <cstdint>

#include "int.h"

namespace xtream {

using u8 = StrictInt<uint8_t>;
using u16 = StrictInt<uint16_t>;
using u32 = StrictInt<uint32_t>;
using u64 = StrictInt<uint64_t>;
using usize = StrictInt<std::size_t>;

constexpr u8 operator""_u8(unsigned long long v) {
    return u8(static_cast<uint8_t>(v));
}
constexpr u16 operator""_u16(unsigned long long v) {
    return u16(static_cast<uint16_t>(v));
}
constexpr u32 operator""_u32(unsigned long long v) {
    return u32(static_cast<uint32_t>(v));
}
constexpr u64 operator""_u64(unsigned long long v) {
    return u64(static_cast<uint64_t>(v));
}
constexpr usize operator""_usize(unsigned long long v) {
    return usize(static_cast<std::size_t>(v));
}

}  // namespace xtream
