#pragma once

#include "core/event.h"

namespace extream {

template <typename F, typename OUT>
concept SourceFunction = requires(F& f) {
    { f() } -> std::convertible_to<Event<OUT>>;
};

}  // namespace extream
