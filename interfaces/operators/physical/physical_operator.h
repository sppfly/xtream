#pragma once

#include "operator_context.h"

namespace extream {

class PhysicalOperator {
public:
    virtual ~PhysicalOperator() = default;

    virtual void setup(OperatorContext& ctx) = 0;
    virtual void open(OperatorContext& ctx) = 0;
    virtual void execute(OperatorContext& ctx, Event<Record>& record) = 0;
    virtual void close(OperatorContext& ctx) = 0;
    virtual void terminate(OperatorContext& ctx) = 0;
};

}  // namespace extream
