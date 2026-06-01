#pragma once

#include <functional>

#include "operators/physical/physical_operator.h"

namespace xtream {

class SinkPhysicalOperator : public PhysicalOperator {
public:
    using Func = std::function<void(Event<Record>&)>;

    explicit SinkPhysicalOperator(Func func) : func_(std::move(func)) {}

    void setup() override {}
    void open() override {}
    void execute(Event<Record>& record) override { func_(record); }
    void close() override {}
    void terminate() override {}

private:
    Func func_;
};

}  // namespace xtream
