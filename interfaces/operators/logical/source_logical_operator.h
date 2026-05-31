#pragma once

#include <string_view>

namespace extream {

class SourceLogicalOperatorImpl {
public:
    std::string_view type_name() const { return "Source"; }
};

}  // namespace extream
