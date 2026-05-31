#pragma once

#include <vector>

#include "operators/physical/physical_operator.h"

namespace extream {

class CollectSinkPhysicalOperator : public PhysicalOperator {
public:
    using Data = std::vector<Event<Record>>;

    void setup() override {}
    void open() override {}
    void execute(Event<Record>& record) override { collected_.push_back(std::move(record)); }
    void close() override {}
    void terminate() override {}

    const Data& collected() const { return collected_; }

private:
    Data collected_;
};

}  // namespace extream
