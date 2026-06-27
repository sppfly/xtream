#pragma once

#include <memory>

#include "core/event.h"
#include "core/record.h"
#include "runtime/stream_element.h"

namespace xtream {

class PhysicalOperator {
public:
    virtual ~PhysicalOperator() = default;

    virtual void setup() = 0;
    virtual void open() = 0;
    virtual void execute(StreamElement& elem) = 0;
    virtual void close() = 0;
    virtual void terminate() = 0;
    virtual bool is_done() const { return false; }
    virtual bool is_window() const { return false; }

    void set_next(std::shared_ptr<PhysicalOperator> next) { next_ = std::move(next); }
    std::shared_ptr<PhysicalOperator> next() const { return next_; }

protected:
    std::shared_ptr<PhysicalOperator> next_;
};

}  // namespace xtream
