#pragma once

#include <functional>

#include "operators/physical/physical_operator.h"

namespace extream {

class SourcePhysicalOperator : public PhysicalOperator {
public:
    using Func = std::function<Event<Record>()>;

    explicit SourcePhysicalOperator(Func func) : func_(std::move(func)) {}

    void setup() override {}
    void open() override {}
    void execute(Event<Record>&) override {
        auto event = func_();
        if (next_) {
            next_->execute(event);
        }
    }
    void close() override {}
    void terminate() override {}

private:
    Func func_;
};

}  // namespace extream
