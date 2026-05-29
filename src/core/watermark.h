#pragma once

#include <algorithm>
#include <compare>

#include "core/types/types.h"

namespace extream {

class Watermark {
public:
    Watermark(i64 timestamp, u64 source_id) : timestamp_(timestamp), source_id_(source_id) {}

    i64 timestamp() const { return timestamp_; }
    u64 source_id() const { return source_id_; }

    auto operator<=>(const Watermark&) const = default;

    static i64 min_timestamp(i64 a, i64 b) { return std::min(a, b); }

private:
    i64 timestamp_;
    u64 source_id_;
};

}  // namespace extream
