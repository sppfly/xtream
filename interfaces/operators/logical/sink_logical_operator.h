#pragma once

#include <string_view>

namespace extream {

class SinkLogicalOperatorImpl {
public:
    std::string_view type_name() const { return "Sink"; }
};

}  // namespace extream
