#pragma once

#include <memory>

#include "core/event.h"
#include "core/record.h"

namespace extream {

class PhysicalOperator {
public:
    virtual ~PhysicalOperator() = default;

    virtual void setup() = 0;
    virtual void open() = 0;
    virtual void execute(Event<Record>& record) = 0;
    virtual void close() = 0;
    virtual void terminate() = 0;

    void set_next(std::shared_ptr<PhysicalOperator> next) { next_ = std::move(next); }
    PhysicalOperator* next() const { return next_.get(); }

protected:
    std::shared_ptr<PhysicalOperator> next_;
};

}  // namespace extream
