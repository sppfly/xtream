#pragma once

#include <memory>
#include <vector>

#include "engine/slot.h"

namespace xtream {

class SlotManager {
public:
    explicit SlotManager(usize slot_count);

    Slot& get_slot(usize index);
    usize slot_count() const { return usize{slots_.size()}; }
    void start_all();
    void stop_all();

private:
    std::vector<std::unique_ptr<Slot>> slots_;
};

}  // namespace xtream
