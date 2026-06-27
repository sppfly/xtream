#pragma once

#include <algorithm>
#include <vector>

#include "core/event.h"
#include "core/record.h"
#include "core/types/types.h"

namespace xtream {

class EventBuffer {
public:
    void insert(Event<Record> event) {
        auto pos = std::ranges::lower_bound(events_, event.timestamp(), std::ranges::less{},
                                            &Event<Record>::timestamp);
        events_.insert(pos, std::move(event));
    }

    std::vector<Event<Record>> extract(u64 start, u64 end) {
        auto lo = std::ranges::lower_bound(events_, start, std::ranges::less{},
                                           &Event<Record>::timestamp);
        auto hi =
            std::ranges::lower_bound(events_, end, std::ranges::less{}, &Event<Record>::timestamp);

        std::vector<Event<Record>> result;
        result.reserve(static_cast<std::size_t>(hi - lo));
        for (auto it = lo; it != hi; ++it) {
            result.push_back(std::move(*it));
        }
        events_.erase(lo, hi);
        return result;
    }

    void cleanup_before(u64 timestamp) {
        auto pos = std::ranges::lower_bound(events_, timestamp, std::ranges::less{},
                                            &Event<Record>::timestamp);
        events_.erase(events_.begin(), pos);
    }

    usize size() const { return usize(events_.size()); }
    bool empty() const { return events_.empty(); }

private:
    std::vector<Event<Record>> events_;
};

}  // namespace xtream
