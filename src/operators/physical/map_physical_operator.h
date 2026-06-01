#pragma once

#include <functional>

#include "operators/physical/physical_operator.h"

namespace xtream {

class MapPhysicalOperator : public PhysicalOperator {
public:
    using Func = std::function<Event<Record>(Event<Record>&)>;

    explicit MapPhysicalOperator(Func func) : func_(std::move(func)) {}

    void setup() override {}
    void open() override {}
    void execute(Event<Record>& record) override {
        auto result = func_(record);
        if (next_) {
            next_->execute(result);
        }
    }
    void close() override {}
    void terminate() override {}

private:
    Func func_;
};

}  // namespace xtream
