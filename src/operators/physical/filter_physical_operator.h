#pragma once

#include <functional>

#include "operators/physical/physical_operator.h"

namespace extream {

class FilterPhysicalOperator : public PhysicalOperator {
public:
    using Pred = std::function<bool(Event<Record>&)>;

    explicit FilterPhysicalOperator(Pred pred) : pred_(std::move(pred)) {}

    void setup(OperatorContext&) override {}
    void open(OperatorContext&) override {}
    void execute(OperatorContext& ctx, Event<Record>& record) override {
        if (pred_(record)) {
            ctx.emit(record);
        }
    }
    void close(OperatorContext&) override {}
    void terminate(OperatorContext&) override {}

private:
    Pred pred_;
};

}  // namespace extream
