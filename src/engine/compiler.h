#pragma once

#include <memory>
#include <unordered_map>

#include "graph/dataflow_graph.h"
#include "operators/physical/physical_operator.h"

namespace xtream {

class Compiler {
public:
    static std::shared_ptr<PhysicalOperator> compile(const DataflowGraph& graph) {
        std::unordered_map<OperatorId, std::shared_ptr<PhysicalOperator>> compiled;
        auto order = graph.topological_order();

        for (auto id : order) {
            compiled[id] = graph.op(id).compile();
        }

        for (const auto& edge : graph.edges()) {
            compiled[edge.source()]->set_next(compiled[edge.target()]);
        }

        return compiled[order.front()];
    }
};

}  // namespace xtream
