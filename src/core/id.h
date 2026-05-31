// File: core/id.h

// ... (existing Id template)

#pragma once

#include <compare>
#include <functional>

#include "core/types/types.h"

namespace extream {

template <typename Tag>
class Id {
public:
    constexpr explicit Id(u64 value) : value_(value) {}

    [[nodiscard]] constexpr u64 raw() const { return value_; }

    constexpr auto operator<=>(const Id&) const = default;

private:
    u64 value_;
};

struct OperatorTag {};
using OperatorId = Id<OperatorTag>;

struct EdgeTag {};
using EdgeId = Id<EdgeTag>;

}  // namespace extream

template <typename Tag>
struct std::hash<extream::Id<Tag>> {
    size_t operator()(const extream::Id<Tag>& id) const noexcept {
        return std::hash<uint64_t>{}(id.raw().raw());
    }
};
