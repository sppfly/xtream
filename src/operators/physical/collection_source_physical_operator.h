#pragma once

#include <vector>

#include "operators/physical/physical_operator.h"

namespace extream {

class CollectionSourcePhysicalOperator : public PhysicalOperator {
public:
    using Data = std::vector<Event<Record>>;

    explicit CollectionSourcePhysicalOperator(Data data) : data_(std::move(data)) {}

    void setup(OperatorContext&) override {}
    void open(OperatorContext&) override { idx_ = 0; }
    void execute(OperatorContext& ctx, Event<Record>&) override {
        if (idx_ < data_.size()) {
            auto copy = data_[idx_];
            ++idx_;
            ctx.emit(std::move(copy));
        }
    }
    void close(OperatorContext&) override {}
    void terminate(OperatorContext&) override {}

    const Data& data() const { return data_; }

private:
    Data data_;
    size_t idx_ = 0;
};

}  // namespace extream
