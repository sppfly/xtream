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

    std::string_view type_name() const override { return "ChannelSource"; }
    std::shared_ptr<InputChannel> channel() const { return channel_; }

    void setup() override {}
    void open() override {}

    void execute(StreamElement&) override {
        auto event = channel_->read();
        if (event && next_) {
            StreamElement out(event.value());
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
