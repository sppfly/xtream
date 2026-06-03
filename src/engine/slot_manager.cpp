#include "engine/slot_manager.h"

namespace xtream {

SlotManager::SlotManager(usize slot_count) {
    for (usize i{0_usize}; i < slot_count; ++i) {
        slots_.emplace_back(new Slot());
    }
}

Slot& SlotManager::get_slot(usize index) {
    return *slots_[index.raw()];
}

void SlotManager::start_all() {
    for (auto& slot : slots_) {
        slot->start();
    }
}

void SlotManager::stop_all() {
    for (auto& slot : slots_) {
        slot->stop();
    }
}

}  // namespace xtream
