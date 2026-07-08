#pragma once

#include <algorithm>
#include <functional>
#include <optional>
#include <utility>

#include "core/types/types.h"
#include "core/watermark.h"
#include "operators/physical/physical_operator.h"

namespace xtream {

class SourcePhysicalOperator : public PhysicalOperator {
public:
    using Func = std::function<std::optional<Event<Record>>()>;

    explicit SourcePhysicalOperator(Func func, u64 allowed_lateness = u64(0),
                                    usize watermark_interval = usize(100))
        : func_(std::move(func)),
          allowed_lateness_(allowed_lateness),
          watermark_interval_(watermark_interval) {}

    std::string_view type_name() const override { return "Source"; }

    void setup() override {}
    void open() override {}
    void execute(StreamElement&) override {
        auto event = func_();
        if (!event) {
            done_ = true;
            return;
        }

        max_ts_ = std::max(max_ts_, event->timestamp());

        if (next_) {
            StreamElement out(*event);
            next_->execute(out);
        }

        events_since_watermark_++;
        if (events_since_watermark_ >= watermark_interval_) {
            emit_watermark();
            events_since_watermark_ = usize(0);
        }
    }
    void close() override {
        if (events_since_watermark_ > usize(0)) {
            emit_watermark();
        }
    }
    void terminate() override {}
    bool is_done() const override { return done_; }

private:
    void emit_watermark() {
        if (max_ts_ < allowed_lateness_) return;
        auto wm_ts = max_ts_ - allowed_lateness_;
        if (!watermark_emitted_ || wm_ts > last_watermark_ts_) {
            last_watermark_ts_ = wm_ts;
            watermark_emitted_ = true;
            if (next_) {
                StreamElement out(Watermark(wm_ts, source_id_));
                next_->execute(out);
            }
        }
    }

    Func func_;
    bool done_{false};

    u64 allowed_lateness_;
    usize watermark_interval_;
    usize events_since_watermark_{0};
    u64 max_ts_{0};
    u64 last_watermark_ts_{0};
    bool watermark_emitted_{false};
    u64 source_id_{0};
};

}  // namespace xtream
