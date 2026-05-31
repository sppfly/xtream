#pragma once

#include <vector>

#include "operators/physical/physical_operator.h"

namespace extream {

class CollectionSourcePhysicalOperator : public PhysicalOperator {
public:
    using Data = std::vector<Event<Record>>;

    explicit CollectionSourcePhysicalOperator(Data data) : data_(std::move(data)) {}

    void setup() override {}
    void open() override { idx_ = 0; }
    void execute(Event<Record>&) override {
        if (idx_ < data_.size()) {
            auto copy = data_[idx_];
            ++idx_;
            if (next_) {
                next_->execute(copy);
            }
        }
    }
    void close() override {}
    void terminate() override {}

    const Data& data() const { return data_; }

private:
    Data data_;
    size_t idx_ = 0;
};

}  // namespace extream
