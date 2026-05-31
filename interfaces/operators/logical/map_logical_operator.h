#pragma once

#include <functional>
#include <string_view>

#include "core/event.h"
#include "core/record.h"

namespace extream {

class MapLogicalOperatorImpl {
public:
    using Func = std::function<Event<Record>(Event<Record>&)>;

    explicit MapLogicalOperatorImpl(Func func) : func_(std::move(func)) {}

    std::string_view type_name() const { return "Map"; }

    const Func& function() const { return func_; }

private:
    Func func_;
};

}  // namespace extream
