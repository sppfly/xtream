#pragma once

#include "core/types/uint.h"

namespace xtream {

enum class WindowType { Count, Time };

struct WindowSpec {
    usize size;
    usize advance;
    WindowType type;
};

inline WindowSpec count_window(usize size, usize advance) {
    return {size, advance, WindowType::Count};
}

inline WindowSpec count_tumbling_window(usize size) {
    return {size, size, WindowType::Count};
}

inline WindowSpec time_window(usize size, usize advance) {
    return {size, advance, WindowType::Time};
}

inline WindowSpec time_tumbling_window(usize size) {
    return {size, size, WindowType::Time};
}

}  // namespace xtream
