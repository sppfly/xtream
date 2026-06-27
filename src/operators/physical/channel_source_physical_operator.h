#pragma once

#include <memory>
#include <optional>

#include "operators/physical/physical_operator.h"
#include "runtime/input_channel.h"

namespace xtream {

class ChannelSourcePhysicalOperator : public PhysicalOperator {
public:
    explicit ChannelSourcePhysicalOperator(std::shared_ptr<InputChannel> channel)
        : channel_(std::move(channel)) {}

    void setup() override {}
    void open() override {}

    void execute(StreamElement&) override {
        auto event = channel_->read();
        if (event && next_) {
            StreamElement out(*event);
            next_->execute(out);
        } else {
            done_ = true;
        }
    }

    void close() override {}
    void terminate() override {}
    bool is_done() const override { return done_; }

private:
    std::shared_ptr<InputChannel> channel_;
    bool done_{false};
};

}  // namespace xtream
