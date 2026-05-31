#pragma once

#include "core/event.h"

namespace extream {

template <typename F, typename T>
concept FilterFunction = requires(F& f, Event<T>& e) {
    { f(e) } -> std::same_as<bool>;
};

}  // namespace extream
