#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "core/event.h"
#include "core/record.h"
#include "core/types/window_spec.h"
#include "operators/physical/physical_operator.h"
#include "runtime/input_channel.h"

namespace xtream {

class WindowPhysicalOperator : public PhysicalOperator {
public:
    using Func = std::function<Event<Record>(const std::vector<Event<Record>>&)>;

    WindowPhysicalOperator(WindowSpec spec, Func func) : spec_(spec), func_(std::move(func)) {}

    void setup() override {}
    void open() override {}

    void execute(StreamElement& elem) override {
        if (auto* event = std::get_if<Event<Record>>(&elem)) {
            buffer_.push_back(*event);

            if (spec_.type == WindowType::Count) {
                execute_count();
            } else {
                execute_time(*event);
            }
        }
        // Watermark: 触发逻辑留到下一步
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

    bool is_window() const override { return true; }

    void set_output_channel(std::shared_ptr<InputChannel> channel) {
        output_channel_ = std::move(channel);
    }

private:
    void execute_count() {
        if (buffer_.size() == spec_.size.raw()) {
            fire_and_slide();
        }
    }

    void execute_time(const Event<Record>& record) {
        // maybe find a good window starttime, or give user the freedom to set the window assigner
        if (!window_initialized_) {
            window_start_ = record.timestamp();
            window_end_ = window_start_ + spec_.size;
            window_initialized_ = true;
        }

        if (record.timestamp() >= window_end_) {
            fire_and_slide();
        }
    }

    void fire_and_slide() {
        auto result = func_(buffer_);
        if (output_channel_) {
            output_channel_->write(std::move(result));
        } else if (next_) {
            StreamElement out(result);
            next_->execute(out);
        }

        if (spec_.type == WindowType::Count) {
            auto advance = spec_.advance.raw();
            if (advance >= buffer_.size()) {
                buffer_.clear();
            } else {
                buffer_.erase(buffer_.begin(),
                              buffer_.begin() + static_cast<std::ptrdiff_t>(advance));
            }
        } else if (window_initialized_) {
            window_start_ += spec_.advance;
            window_end_ = window_start_ + spec_.size;
            std::erase_if(buffer_, [this](const auto& e) { return e.timestamp() < window_start_; });
        }
    }

    WindowSpec spec_;
    Func func_;
    std::vector<Event<Record>> buffer_;
    std::shared_ptr<InputChannel> output_channel_;

    bool window_initialized_{false};
    u64 window_start_{0};
    u64 window_end_{0};
};

}  // namespace xtream
