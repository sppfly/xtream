#pragma once

#include "operators/physical/operator_context.h"
#include "operators/physical/physical_operator.h"

namespace extream {

class ChainContext : public OperatorContext {
public:
    ChainContext() = default;

    void set_downstream(PhysicalOperator& op, OperatorContext& ctx) {
        downstream_ = &op;
        downstream_ctx_ = &ctx;
    }

    void emit(Event<Record> event) override {
        if (downstream_ && downstream_ctx_) {
            downstream_->execute(*downstream_ctx_, event);
        }
    }

    i64 get_watermark() const override { return i64(0); }

private:
    PhysicalOperator* downstream_ = nullptr;
    OperatorContext* downstream_ctx_ = nullptr;
};

}  // namespace extream
