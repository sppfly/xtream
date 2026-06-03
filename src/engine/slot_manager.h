#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "engine/slot.h"

namespace xtream {

class SlotManager {
public:
    explicit SlotManager(size_t slot_count);

    Slot& get_slot(size_t index);
    size_t slot_count() const { return slots_.size(); }
    void start_all();
    void stop_all();

private:
    std::vector<std::unique_ptr<Slot>> slots_;
};

}  // namespace xtream
