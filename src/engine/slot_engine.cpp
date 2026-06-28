#include "engine/slot_engine.h"

#include "core/types/types.h"

namespace xtream {

SlotEngine::SlotEngine(usize slot_count) : slot_manager_(slot_count) {}

// TODO: we don't need this method anymore, should remove it later
void SlotEngine::submit(DataflowGraph graph) {
    Pipeline pipeline(std::move(graph));
    pipelines_.push_back(std::move(pipeline));
}

void SlotEngine::submit(Pipeline pipeline) {
    pipelines_.push_back(std::move(pipeline));
}

void SlotEngine::execute() {
    running_ = true;

    for (usize i{0_usize}; i < usize{pipelines_.size()} && i < slot_manager_.slot_count(); ++i) {
        slot_manager_.get_slot(i).assign(std::move(pipelines_[i.raw()]));
    }

    slot_manager_.start_all();

    while (running_) {
        bool all_done = true;
        for (usize i{0_usize}; i < slot_manager_.slot_count(); ++i) {
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
