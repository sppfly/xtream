#pragma once

#include "core/event.h"
#include "core/record.h"

namespace extream {

class OperatorContext {
public:
    virtual ~OperatorContext() = default;

    virtual void emit(Event<Record> event) = 0;
    virtual i64 get_watermark() const = 0;
};

}  // namespace extream
