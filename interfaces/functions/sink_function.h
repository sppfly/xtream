#pragma once

#include "core/event.h"

namespace extream {

template <typename F, typename IN>
concept SinkFunction = requires(F& f, Event<IN>& e) {
    { f(e) } -> std::same_as<void>;
};

}  // namespace extream
