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
    void execute(StreamElement& elem) override {
        if (auto* event = std::get_if<Event<Record>>(&elem)) {
            func_(*event);
        }
        // Watermark: 忽略
    }
    void close() override {}
    void terminate() override {}

private:
    Func func_;
};

}  // namespace xtream
