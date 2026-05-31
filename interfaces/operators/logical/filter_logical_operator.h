#pragma once

#include <functional>
#include <string_view>

#include "core/event.h"
#include "core/record.h"

namespace extream {

class FilterLogicalOperatorImpl {
public:
    using Func = std::function<bool(Event<Record>&)>;

    explicit FilterLogicalOperatorImpl(Func func) : func_(std::move(func)) {}

    std::string_view type_name() const { return "Filter"; }

    const Func& function() const { return func_; }

private:
    Func func_;
};

}  // namespace extream
