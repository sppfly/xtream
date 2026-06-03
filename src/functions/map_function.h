#pragma once

#include "core/event.h"

namespace xtream {

template <typename F, typename IN, typename OUT>
concept MapFunction = requires(F& f, Event<IN>& e) {
    { f(e) } -> std::same_as<Event<OUT>>;
};

}  // namespace xtream
