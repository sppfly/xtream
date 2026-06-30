#pragma once

#include <functional>
#include <vector>

#include "core/event.h"
#include "core/record.h"
#include "core/types/window_spec.h"
#include "operators/physical/window_physical_operator.h"
#include "runtime/stream_element.h"

namespace xtream {

class CountWindowPhysicalOperator : public WindowPhysicalOperator {
public:
    CountWindowPhysicalOperator(WindowSpec spec, Func func) : spec_(spec), func_(std::move(func)) {}

    void setup() override {}
    void open() override {}

    void execute(StreamElement& elem) override {
        if (auto* event = std::get_if<Event<Record>>(&elem)) {
            buffer_.push_back(*event);
            if (buffer_.size() == spec_.size) {
                fire_and_slide();
            }
        }
    }

    void close() override {
        if (output_channel_ && !buffer_.empty()) {
            auto result = func_(buffer_);
            output_channel_->write(std::move(result));
        }
        if (output_channel_) {
            output_channel_->close();
        }
    }

    void terminate() override {}

private:
    void fire_and_slide() {
        auto result = func_(buffer_);
        if (output_channel_) {
            output_channel_->write(std::move(result));
        } else if (next_) {
            StreamElement out(result);
            next_->execute(out);
        }

        auto advance = spec_.advance;
        if (advance >= buffer_.size()) {
            buffer_.clear();
        } else {
            buffer_.erase(buffer_.begin(), buffer_.begin() + advance);
        }
    }

    WindowSpec spec_;
    Func func_;
    std::vector<Event<Record>> buffer_;
};

}  // namespace xtream
