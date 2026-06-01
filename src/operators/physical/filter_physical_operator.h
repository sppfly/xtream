#pragma once

#include <functional>

#include "operators/physical/physical_operator.h"

namespace xtream {

class FilterPhysicalOperator : public PhysicalOperator {
public:
    using Pred = std::function<bool(Event<Record>&)>;

    explicit FilterPhysicalOperator(Pred pred) : pred_(std::move(pred)) {}

    void setup() override {}
    void open() override {}
    void execute(Event<Record>& record) override {
        if (pred_(record)) {
            if (next_) {
                next_->execute(record);
            }
        }
    }
    void close() override {}
    void terminate() override {}

private:
    Pred pred_;
};

}  // namespace xtream
