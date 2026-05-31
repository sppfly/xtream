#pragma once

#include <memory>
#include <string_view>

#include "operators/physical/collect_sink_physical_operator.h"

namespace extream {

class SinkLogicalOperatorImpl {
public:
    std::string_view type_name() const { return "Sink"; }

    std::shared_ptr<PhysicalOperator> compile() const {
        return std::make_shared<CollectSinkPhysicalOperator>();
    }
};

}  // namespace extream
