#pragma once

#include <memory>
#include <vector>

#include "engine/compiler.h"
#include "engine/pipeline.h"
#include "graph/dataflow_graph.h"
#include "operators/physical/channel_source_physical_operator.h"
#include "operators/physical/physical_operator.h"
#include "operators/physical/window_physical_operator.h"
#include "runtime/input_channel.h"

namespace xtream {

class ExecutionGraph {
public:
    static std::vector<Pipeline> build(const DataflowGraph& graph) {
        auto root = Compiler::compile(graph);
        std::vector<std::shared_ptr<PhysicalOperator>> roots;
        std::vector<Pipeline> pipelines;

        auto current = root;
        while (current) {
            auto node = current;
            std::shared_ptr<PhysicalOperator> window_node;

            while (node) {
                if (node->is_window()) {
                    window_node = node;
                    break;
                }
                node = node->next();
            }

            if (!window_node) {
                roots.push_back(current);
                break;
            }

            roots.push_back(current);

            auto channel = std::make_shared<InputChannel>();
            auto downstream = window_node->next();

            static_cast<WindowPhysicalOperator*>(window_node.get())->set_output_channel(channel);
            window_node->set_next(nullptr);

            auto channel_source = std::make_shared<ChannelSourcePhysicalOperator>(channel);
            if (downstream) {
                channel_source->set_next(downstream);
            }

            current = channel_source;
        }

        for (auto& r : roots) {
            pipelines.emplace_back(r);
        }

        return pipelines;
    }
};

}  // namespace xtream
