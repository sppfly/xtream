#pragma once

#include <variant>

#include "core/event.h"
#include "core/record.h"
#include "core/watermark.h"

namespace xtream {

using StreamElement = std::variant<Event<Record>, Watermark>;

}  // namespace xtream
