#pragma once

#include <atomic>
#include <vector>

#include "core/types/types.h"
#include "engine/execution_engine.h"
#include "engine/pipeline.h"
#include "engine/slot_manager.h"

namespace xtream {

class SlotEngine : public ExecutionEngine {
public:
    explicit SlotEngine(usize slot_count);

    void submit(DataflowGraph graph) override;
    void submit(Pipeline pipeline);
    void execute() override;
    void cancel() override;

private:
    SlotManager slot_manager_;
    std::vector<Pipeline> pipelines_;
    std::atomic<bool> running_{false};
};

}  // namespace xtream
