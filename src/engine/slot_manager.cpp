#include "engine/slot_manager.h"

namespace xtream {

SlotManager::SlotManager(size_t slot_count) {
    for (size_t i = 0; i < slot_count; ++i) {
        slots_.emplace_back(new Slot());
    }
}

Slot& SlotManager::get_slot(size_t index) {
    return *slots_[index];
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
