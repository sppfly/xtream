#pragma once

#include <functional>
#include <memory>

#include "operators/physical/physical_operator.h"
#include "runtime/input_channel.h"

namespace xtream {

class WindowPhysicalOperator : public PhysicalOperator {
public:
    using Func = std::function<Event<Record>(const std::vector<Event<Record>>&)>;
    bool is_window() const override { return true; }
    void set_output_channel(std::shared_ptr<InputChannel> channel) {
        output_channel_ = std::move(channel);
    }

protected:
    std::shared_ptr<InputChannel> output_channel_;
};

}  // namespace xtream