#pragma once

#include <atomic>
#include <vector>

#include "engine/execution_engine.h"
#include "engine/pipeline.h"
#include "engine/slot_manager.h"

namespace xtream {

class SlotEngine : public ExecutionEngine {
public:
    explicit SlotEngine(size_t slot_count);

    void submit(DataflowGraph graph) override;
    void execute() override;
    void cancel() override;

private:
    SlotManager slot_manager_;
    std::vector<Pipeline> pipelines_;
    std::atomic<bool> running_{false};
};

}  // namespace xtream
