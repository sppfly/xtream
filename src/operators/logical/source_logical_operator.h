#pragma once

#include <functional>
#include <memory>
#include <string_view>

#include "core/event.h"
#include "core/record.h"
#include "core/types/types.h"
#include "operators/physical/source_physical_operator.h"

namespace xtream {

class SourceLogicalOperator {
public:
    using Func = std::function<std::optional<Event<Record>>()>;

    explicit SourceLogicalOperator(Func func) : func_(std::move(func)) {}

    std::string_view type_name() const { return "Source"; }
    const Func& function() const { return func_; }

    u64 allowed_lateness() const { return allowed_lateness_; }
    void set_allowed_lateness(u64 lateness) { allowed_lateness_ = lateness; }

    usize watermark_interval() const { return watermark_interval_; }
    void set_watermark_interval(usize interval) { watermark_interval_ = interval; }

    std::shared_ptr<PhysicalOperator> compile() const {
        return std::make_shared<SourcePhysicalOperator>(func_, allowed_lateness_,
                                                        watermark_interval_);
    }

private:
    Func func_;
    u64 allowed_lateness_{0};
    usize watermark_interval_{100};
};

}  // namespace xtream
