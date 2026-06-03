#pragma once

#include "graph/dataflow_graph.h"

namespace xtream {

class ExecutionEngine {
public:
    virtual ~ExecutionEngine() = default;
    virtual void submit(DataflowGraph graph) = 0;
    virtual void execute() = 0;
    virtual void cancel() = 0;
};

}  // namespace xtream
