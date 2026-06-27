#pragma once

#include <functional>
#include <variant>

#include "operators/physical/physical_operator.h"

namespace xtream {

class MapPhysicalOperator : public PhysicalOperator {
public:
    using Func = std::function<Event<Record>(Event<Record>&)>;

    explicit MapPhysicalOperator(Func func) : func_(std::move(func)) {}

    void setup() override {}
    void open() override {}
    void execute(StreamElement& elem) override {
        if (auto* event = std::get_if<Event<Record>>(&elem)) {
            auto result = func_(*event);
            if (next_) {
                StreamElement out(result);
                next_->execute(out);
            }
        } else {
            // Watermark: 透传，用户不感知
            if (next_) {
                next_->execute(elem);
            }
        }
    }
    void close() override {}
    void terminate() override {}

private:
    Func func_;
};

}  // namespace xtream
