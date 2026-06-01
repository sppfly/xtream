#pragma once

#include <functional>
#include <memory>
#include <string_view>

#include "core/event.h"
#include "core/record.h"
#include "operators/physical/source_physical_operator.h"

namespace extream {

class SourceLogicalOperator {
public:
    using Func = std::function<Event<Record>()>;

    explicit SourceLogicalOperator(Func func) : func_(std::move(func)) {}

    std::string_view type_name() const { return "Source"; }
    const Func& function() const { return func_; }

    std::shared_ptr<PhysicalOperator> compile() const {
        return std::make_shared<SourcePhysicalOperator>(func_);
    }

private:
    Func func_;
};

}  // namespace extream
