#pragma once

#include <functional>
#include <optional>

#include "operators/physical/physical_operator.h"

namespace xtream {

class SourcePhysicalOperator : public PhysicalOperator {
public:
    using Func = std::function<std::optional<Event<Record>>()>;

    explicit SourcePhysicalOperator(Func func) : func_(std::move(func)) {}

    void setup() override {}
    void open() override {}
    void execute(StreamElement&) override {
        auto event = func_();
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
    Func func_;
    bool done_{false};
};

}  // namespace xtream
