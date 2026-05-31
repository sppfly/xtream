#pragma once

#include <vector>

#include "operators/physical/physical_operator.h"

namespace extream {

class CollectSinkPhysicalOperator : public PhysicalOperator {
public:
    using Data = std::vector<Event<Record>>;

    void setup(OperatorContext&) override {}
    void open(OperatorContext&) override {}
    void execute(OperatorContext&, Event<Record>& record) override {
        collected_.push_back(std::move(record));
    }
    void close(OperatorContext&) override {}
    void terminate(OperatorContext&) override {}

    const Data& collected() const { return collected_; }

private:
    Data collected_;
};

}  // namespace extream
