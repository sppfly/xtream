#pragma once

#include <functional>
#include <memory>
#include <string_view>
#include <vector>

#include "core/event.h"
#include "core/record.h"
#include "core/types/window_spec.h"
#include "operators/physical/count_window_physical_operator.h"
#include "operators/physical/time_window_physical_operator.h"

namespace xtream {

class WindowLogicalOperator {
public:
    using Func = std::function<Event<Record>(const std::vector<Event<Record>>&)>;

    WindowLogicalOperator(WindowSpec spec, Func func) : spec_(spec), func_(std::move(func)) {}

    std::string_view type_name() const { return "Window"; }
    const WindowSpec& spec() const { return spec_; }
    const Func& function() const { return func_; }

    std::shared_ptr<PhysicalOperator> compile() const {
        if (spec_.type == WindowType::Count) {
            return std::make_shared<CountWindowPhysicalOperator>(spec_, func_);
        }
        return std::make_shared<TimeWindowPhysicalOperator>(spec_, func_);
    }

private:
    WindowSpec spec_;
    Func func_;
};

}  // namespace xtream
