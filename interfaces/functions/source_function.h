#pragma once

#include "core/event.h"

namespace extream {

template <typename OUT>
struct SourceEmitter {
    void emit(Event<OUT> event);
};

template <typename F, typename OUT>
concept SourceFunction = requires(F& f, SourceEmitter<OUT>& emitter) {
    { f(emitter) } -> std::same_as<void>;
};

}  // namespace extream
