#include "engine/slot_engine.h"

namespace xtream {

SlotEngine::SlotEngine(size_t slot_count) : slot_manager_(slot_count) {}

void SlotEngine::submit(DataflowGraph graph) {
    Pipeline pipeline(std::move(graph));
    pipelines_.push_back(std::move(pipeline));
}

void SlotEngine::execute() {
    running_ = true;

    for (size_t i = 0; i < pipelines_.size() && i < slot_manager_.slot_count(); ++i) {
        slot_manager_.get_slot(i).assign(std::move(pipelines_[i]));
    }

    slot_manager_.start_all();

    while (running_) {
        bool all_done = true;
        for (size_t i = 0; i < slot_manager_.slot_count(); ++i) {
            if (slot_manager_.get_slot(i).is_running()) {
                all_done = false;
                break;
            }
        }
        if (all_done) {
            break;
        }
    }
}

void SlotEngine::cancel() {
    running_ = false;
    slot_manager_.stop_all();
}

}  // namespace xtream
