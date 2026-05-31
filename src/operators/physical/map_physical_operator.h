#pragma once

#include <functional>

#include "operators/physical/physical_operator.h"

namespace extream {

class MapPhysicalOperator : public PhysicalOperator {
public:
    using Func = std::function<Event<Record>(Event<Record>&)>;

    explicit MapPhysicalOperator(Func func) : func_(std::move(func)) {}

    void setup(OperatorContext&) override {}
    void open(OperatorContext&) override {}
    void execute(OperatorContext& ctx, Event<Record>& record) override {
        auto result = func_(record);
        ctx.emit(std::move(result));
    }
    void close(OperatorContext&) override {}
    void terminate(OperatorContext&) override {}

private:
    Func func_;
};

}  // namespace extream
