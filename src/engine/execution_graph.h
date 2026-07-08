#pragma once

#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
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

    static std::string to_graphviz(const std::vector<Pipeline>& pipelines) {
        // Map each InputChannel to a unique id for cross-pipeline edges.
        std::unordered_map<InputChannel*, size_t> channel_id;
        for (const auto& p : pipelines) {
            for (auto node = p.root(); node; node = node->next()) {
                if (auto* w = dynamic_cast<WindowPhysicalOperator*>(node.get())) {
                    if (w->output_channel()) {
                        channel_id[w->output_channel().get()] = channel_id.size();
                    }
                }
            }
        }

        std::ostringstream out;
        out << "digraph ExecutionGraph {\n";
        out << "  rankdir=LR;\n";
        out << "  compound=true;\n";
        out << "  node [shape=box, fontname=\"Helvetica\"];\n";
        out << "  edge [fontname=\"Helvetica\"];\n";

        for (size_t pi = 0; pi < pipelines.size(); ++pi) {
            out << "  subgraph cluster_p" << pi << " {\n";
            out << "    label=\"Pipeline " << pi << "\";\n";
            out << "    style=dashed;\n";
            size_t ni = 0;
            for (auto node = pipelines[pi].root(); node; node = node->next(), ++ni) {
                out << "    p" << pi << "_n" << ni << " [label=\"" << node->type_name() << "\"];\n";
            }
            out << "  }\n";
            // chain edges within pipeline
            size_t last = 0;
            size_t count = 0;
            for (auto node = pipelines[pi].root(); node; node = node->next(), ++count) {
                if (node->next()) {
                    out << "  p" << pi << "_n" << count << " -> p" << pi << "_n" << (count + 1)
                        << ";\n";
                }
                last = count;
            }
            (void)last;
        }

        // cross-pipeline channel edges: window -> channel_source
        for (size_t pi = 0; pi < pipelines.size(); ++pi) {
            size_t ni = 0;
            for (auto node = pipelines[pi].root(); node; node = node->next(), ++ni) {
                auto* w = dynamic_cast<WindowPhysicalOperator*>(node.get());
                if (!w || !w->output_channel()) continue;
                auto it = channel_id.find(w->output_channel().get());
                if (it == channel_id.end()) continue;
                // find the pipeline whose ChannelSource reads this channel
                for (size_t qj = 0; qj < pipelines.size(); ++qj) {
                    size_t mi = 0;
                    for (auto m = pipelines[qj].root(); m; m = m->next(), ++mi) {
                        auto* cs = dynamic_cast<ChannelSourcePhysicalOperator*>(m.get());
                        if (!cs || !cs->channel()) continue;
                        if (cs->channel().get() == w->output_channel().get()) {
                            out << "  p" << pi << "_n" << ni << " -> p" << qj << "_n" << mi
                                << " [style=dashed, label=\"channel " << it->second << "\"];\n";
                        }
                    }
                }
            }
        }

        out << "}\n";
        return out.str();
    }

    static std::string to_json(const std::vector<Pipeline>& pipelines) {
        std::unordered_map<InputChannel*, size_t> channel_id;
        for (const auto& p : pipelines) {
            for (auto node = p.root(); node; node = node->next()) {
                if (auto* w = dynamic_cast<WindowPhysicalOperator*>(node.get())) {
                    if (w->output_channel()) {
                        channel_id[w->output_channel().get()] = channel_id.size();
                    }
                }
            }
        }

        std::ostringstream out;
        out << "{\n  \"pipelines\": [\n";
        for (size_t pi = 0; pi < pipelines.size(); ++pi) {
            out << "    {\n";
            out << "      \"index\": " << pi << ",\n";
            out << "      \"operators\": [\n";
            size_t ni = 0;
            for (auto node = pipelines[pi].root(); node; node = node->next(), ++ni) {
                out << "        {\"index\": " << ni << ", \"type\": \"" << node->type_name()
                    << "\"}";
                if (node->next()) out << ",";
                out << "\n";
            }
            out << "      ]\n";
            out << "    }";
            if (pi + 1 < pipelines.size()) out << ",";
            out << "\n";
        }
        out << "  ],\n";
        out << "  \"channels\": [";
        bool first = true;
        for (size_t pi = 0; pi < pipelines.size(); ++pi) {
            size_t ni = 0;
            for (auto node = pipelines[pi].root(); node; node = node->next(), ++ni) {
                auto* w = dynamic_cast<WindowPhysicalOperator*>(node.get());
                if (!w || !w->output_channel()) continue;
                auto it = channel_id.find(w->output_channel().get());
                if (it == channel_id.end()) continue;
                for (size_t qj = 0; qj < pipelines.size(); ++qj) {
                    size_t mi = 0;
                    for (auto m = pipelines[qj].root(); m; m = m->next(), ++mi) {
                        auto* cs = dynamic_cast<ChannelSourcePhysicalOperator*>(m.get());
                        if (!cs || !cs->channel()) continue;
                        if (cs->channel().get() == w->output_channel().get()) {
                            out << (first ? "\n    " : ",\n    ");
                            first = false;
                            out << "{\"id\": " << it->second << ", \"from\": {\"pipeline\": " << pi
                                << ", \"operator\": " << ni << "}, \"to\": {\"pipeline\": " << qj
                                << ", \"operator\": " << mi << "}}";
                        }
                    }
                }
            }
        }
        out << "\n  ]\n}\n";
        return out.str();
    }
};

}  // namespace xtream
