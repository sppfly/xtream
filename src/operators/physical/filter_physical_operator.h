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
    void execute(StreamElement& elem) override {
        if (auto* event = std::get_if<Event<Record>>(&elem)) {
            if (pred_(*event)) {
                if (next_) {
                    next_->execute(elem);
                }
            }
        } else {
            // Watermark: pass through
            if (next_) {
                next_->execute(elem);
            }
        }
    }
    void close() override {}
    void terminate() override {}

private:
    Pred pred_;
};

}  // namespace xtream
