#pragma once

#include <functional>
#include <memory>
#include <string_view>

#include "core/event.h"
#include "core/record.h"
#include "core/types/int.h"

namespace xtream {

class PhysicalOperator;

class IntervalJoinLogicalOperator {
public:
    using KeySelector = std::function<u64(const Event<Record>&)>;
    using JoinFunc = std::function<Event<Record>(const Event<Record>&, const Event<Record>&)>;

    IntervalJoinLogicalOperator(KeySelector left_key, KeySelector right_key, i64 lower_bound,
                                i64 upper_bound, JoinFunc func)
        : left_key_(std::move(left_key)),
          right_key_(std::move(right_key)),
          lower_bound_(lower_bound),
          upper_bound_(upper_bound),
          func_(std::move(func)) {}

    std::string_view type_name() const { return "IntervalJoin"; }
    const KeySelector& left_key() const { return left_key_; }
    const KeySelector& right_key() const { return right_key_; }
    i64 lower_bound() const { return lower_bound_; }
    i64 upper_bound() const { return upper_bound_; }
    const JoinFunc& function() const { return func_; }

    std::shared_ptr<PhysicalOperator> compile() const { return nullptr; }

private:
    KeySelector left_key_;
    KeySelector right_key_;
    i64 lower_bound_;
    i64 upper_bound_;
    JoinFunc func_;
};

}  // namespace xtream
