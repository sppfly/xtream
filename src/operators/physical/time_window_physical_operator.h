#pragma once

#include <functional>
#include <vector>

#include "core/event.h"
#include "core/record.h"
#include "core/types/window_spec.h"
#include "core/watermark.h"
#include "operators/physical/window_physical_operator.h"
#include "runtime/event_buffer.h"
#include "runtime/stream_element.h"

namespace xtream {

class TimeWindowPhysicalOperator : public WindowPhysicalOperator {
public:
    TimeWindowPhysicalOperator(WindowSpec spec, Func func) : spec_(spec), func_(std::move(func)) {}

    std::string_view type_name() const override { return "TimeWindow"; }

    void setup() override {}
    void open() override {}

    void execute(StreamElement& elem) override {
        if (auto* event = std::get_if<Event<Record>>(&elem)) {
            buffer_.insert(*event);
            if (!window_initialized_) {
                next_window_end_ = spec_.advance;
                window_initialized_ = true;
            }
        } else if (auto* wm = std::get_if<Watermark>(&elem)) {
            on_watermark(*wm);
        }
    }

    void close() override {
        // flush remaining windows until buffer is empty
        if (window_initialized_) {
            while (!buffer_.empty()) {
                auto start =
                    (next_window_end_ > spec_.size) ? (next_window_end_ - spec_.size) : u64(0);
                auto events = buffer_.query(start, next_window_end_);
                if (!events.empty()) {
                    auto result = func_(events);
                    emit(result);
                }
                next_window_end_ = next_window_end_ + spec_.advance;
                // advance past any remaining events to avoid infinite loop
                auto earliest_active_start =
                    (next_window_end_ > spec_.size) ? (next_window_end_ - spec_.size) : u64(0);
                buffer_.cleanup_before(earliest_active_start);
            }
        }
        if (output_channel_) {
            output_channel_->close();
        }
    }

    void terminate() override {}

private:
    void on_watermark(Watermark wm) {
        if (!window_initialized_) return;
        while (next_window_end_ <= wm.timestamp()) {
            auto start = (next_window_end_ > spec_.size) ? (next_window_end_ - spec_.size) : u64(0);
            auto events = buffer_.query(start, next_window_end_);
            if (!events.empty()) {
                auto result = func_(events);
                emit(result);
            }
            // guard against overflow
            if (next_window_end_ > wm.timestamp()) break;
            next_window_end_ = next_window_end_ + spec_.advance;
        }
        if (window_initialized_) {
            auto earliest_active_start =
                (next_window_end_ > spec_.size) ? (next_window_end_ - spec_.size) : u64(0);
            buffer_.cleanup_before(earliest_active_start);
        }
    }

    void emit(Event<Record>& result) {
        if (output_channel_) {
            output_channel_->write(result);
        } else if (next_) {
            StreamElement out(result);
            next_->execute(out);
        }
    }

    WindowSpec spec_;
    Func func_;
    EventBuffer buffer_;
    u64 next_window_end_{0};
    bool window_initialized_{false};
};

}  // namespace xtream
