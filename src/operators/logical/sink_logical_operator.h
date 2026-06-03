#pragma once

#include <functional>
#include <memory>
#include <string_view>

#include "core/event.h"
#include "core/record.h"
#include "operators/physical/sink_physical_operator.h"

namespace xtream {

class SinkLogicalOperator {
public:
    using Func = std::function<void(Event<Record>&)>;

    explicit SinkLogicalOperator(Func func) : func_(std::move(func)) {}

    std::string_view type_name() const { return "Sink"; }
    const Func& function() const { return func_; }

    std::shared_ptr<PhysicalOperator> compile() const {
        return std::make_shared<SinkPhysicalOperator>(func_);
    }

private:
    Func func_;
};

}  // namespace xtream
