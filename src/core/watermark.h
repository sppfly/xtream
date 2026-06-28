#pragma once

#include <algorithm>
#include <compare>

#include "core/types/types.h"

namespace xtream {

class Watermark {
public:
    Watermark(u64 timestamp, u64 source_id) : timestamp_(timestamp), source_id_(source_id) {}

    u64 timestamp() const { return timestamp_; }
    u64 source_id() const { return source_id_; }

    auto operator<=>(const Watermark&) const = default;

    static u64 min_timestamp(u64 a, u64 b) { return std::min(a, b); }

private:
    u64 timestamp_;
    u64 source_id_;
};

}  // namespace xtream
