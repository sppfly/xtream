#pragma once

#include <functional>
#include <memory>
#include <string_view>

#include "core/event.h"
#include "core/record.h"
#include "core/types/window_spec.h"

namespace xtream {

class PhysicalOperator;

class WindowJoinLogicalOperator {
public:
    using KeySelector = std::function<u64(const Event<Record>&)>;
    using JoinFunc = std::function<Event<Record>(const Event<Record>&, const Event<Record>&)>;

    WindowJoinLogicalOperator(KeySelector left_key, KeySelector right_key, WindowSpec spec,
                              JoinFunc func)
        : left_key_(std::move(left_key)),
          right_key_(std::move(right_key)),
          spec_(spec),
          func_(std::move(func)) {}

    std::string_view type_name() const { return "WindowJoin"; }
    const KeySelector& left_key() const { return left_key_; }
    const KeySelector& right_key() const { return right_key_; }
    const WindowSpec& spec() const { return spec_; }
    const JoinFunc& function() const { return func_; }

    std::shared_ptr<PhysicalOperator> compile() const { return nullptr; }

private:
    KeySelector left_key_;
    KeySelector right_key_;
    WindowSpec spec_;
    JoinFunc func_;
};

}  // namespace xtream
