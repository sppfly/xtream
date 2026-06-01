#pragma once

#include <functional>
#include <memory>
#include <string_view>

#include "core/event.h"
#include "core/record.h"
#include "operators/physical/filter_physical_operator.h"

namespace extream {

class FilterLogicalOperator {
public:
    using Func = std::function<bool(Event<Record>&)>;

    explicit FilterLogicalOperator(Func func) : func_(std::move(func)) {}

    std::string_view type_name() const { return "Filter"; }
    const Func& function() const { return func_; }

    std::shared_ptr<PhysicalOperator> compile() const {
        return std::make_shared<FilterPhysicalOperator>(func_);
    }

private:
    Func func_;
};

}  // namespace extream
