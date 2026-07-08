#pragma once

#include <cassert>
#include <deque>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#include "core/id.h"
#include "graph/stream_edge.h"
#include "operators/logical/logical_operator.h"

namespace xtream {

class DataflowGraph {
public:
    DataflowGraph(std::vector<LogicalOperator> operators, std::vector<StreamEdge> edges)
        : operators_(std::move(operators)), edges_(std::move(edges)) {}

    const std::vector<LogicalOperator>& operators() const { return operators_; }
    const std::vector<StreamEdge>& edges() const { return edges_; }

    const LogicalOperator& op(OperatorId id) const {
        for (const auto& op : operators_) {
            if (op.id() == id) {
                return op;
            }
        }
        assert(false);
    }

    const LogicalOperator& op(std::string_view name) const {
        for (const auto& op : operators_) {
            if (op.name() == name) {
                return op;
            }
        }
        assert(false);
    }

    const StreamEdge& edge(EdgeId id) const {
        for (const auto& e : edges_) {
            if (e.id() == id) {
                return e;
            }
        }
        assert(false);
    }

    std::vector<OperatorId> sources_of(OperatorId target) const {
        std::vector<OperatorId> result;
        for (const auto& e : edges_) {
            if (e.target() == target) {
                result.push_back(e.source());
            }
        }
        return result;
    }

    std::vector<OperatorId> targets_of(OperatorId source) const {
        std::vector<OperatorId> result;
        for (const auto& e : edges_) {
            if (e.source() == source) {
                result.push_back(e.target());
            }
        }
        return result;
    }

    std::vector<OperatorId> roots() const {
        std::vector<OperatorId> result;
        for (const auto& op : operators_) {
            if (in_degree_of(op.id()) == 0) {
                result.push_back(op.id());
            }
        }
        return result;
    }

    std::vector<OperatorId> leaves() const {
        std::vector<OperatorId> result;
        for (const auto& op : operators_) {
            if (out_degree_of(op.id()) == 0) {
                result.push_back(op.id());
            }
        }
        return result;
    }

    size_t in_degree_of(OperatorId id) const {
        size_t count = 0;
        for (const auto& e : edges_) {
            if (e.target() == id) {
                count++;
            }
        }
        return count;
    }

    size_t out_degree_of(OperatorId id) const {
        size_t count = 0;
        for (const auto& e : edges_) {
            if (e.source() == id) {
                count++;
            }
        }
        return count;
    }

    std::string to_graphviz() const {
        std::ostringstream out;
        out << "digraph DataflowGraph {\n";
        out << "  rankdir=LR;\n";
        out << "  node [shape=box, fontname=\"Helvetica\"];\n";
        out << "  edge [fontname=\"Helvetica\"];\n";
        for (const auto& op : operators_) {
            out << "  op" << op.id().raw() << " [label=\"" << op.name() << "\\n"
                << "(" << op.type_name() << ", p=" << op.parallelism().raw() << ")\"];";
            out << "\n";
        }
        for (const auto& e : edges_) {
            out << "  op" << e.source().raw() << " -> op" << e.target().raw() << " [label=\""
                << partition_name(e.partition()) << "\"];\n";
        }
        out << "}\n";
        return out.str();
    }

    std::string to_json() const {
        std::ostringstream out;
        out << "{\n";
        out << "  \"operators\": [\n";
        for (size_t i = 0; i < operators_.size(); ++i) {
            const auto& op = operators_[i];
            out << "    {\"id\": " << op.id().raw() << ", \"name\": \"" << op.name() << "\""
                << ", \"type\": \"" << op.type_name() << "\""
                << ", \"parallelism\": " << op.parallelism().raw() << "}";
            if (i + 1 < operators_.size()) out << ",";
            out << "\n";
        }
        out << "  ],\n";
        out << "  \"edges\": [\n";
        for (size_t i = 0; i < edges_.size(); ++i) {
            const auto& e = edges_[i];
            out << "    {\"id\": " << e.id().raw() << ", \"source\": " << e.source().raw()
                << ", \"target\": " << e.target().raw() << ", \"partition\": \""
                << partition_name(e.partition()) << "\"}";
            if (i + 1 < edges_.size()) out << ",";
            out << "\n";
        }
        out << "  ]\n";
        out << "}\n";
        return out.str();
    }

    std::vector<OperatorId> topological_order() const {
        std::unordered_map<OperatorId, std::vector<OperatorId>> adjacency;
        for (const auto& op : operators_) {
            adjacency[op.id()] = {};
        }
        for (const auto& e : edges_) {
            adjacency[e.source()].push_back(e.target());
        }

        std::unordered_map<OperatorId, size_t> in_degree;
        for (const auto& op : operators_) {
            in_degree[op.id()] = 0;
        }
        for (const auto& e : edges_) {
            in_degree[e.target()]++;
        }

        std::deque<OperatorId> queue;
        for (const auto& [id, deg] : in_degree) {
            if (deg == 0) {
                queue.push_back(id);
            }
        }

        std::vector<OperatorId> order;
        while (!queue.empty()) {
            auto id = queue.front();
            queue.pop_front();
            order.push_back(id);
            for (const auto& neighbor : adjacency[id]) {
                in_degree[neighbor]--;
                if (in_degree[neighbor] == 0) {
                    queue.push_back(neighbor);
                }
            }
        }

        return order;
    }

private:
    static std::string_view partition_name(EdgePartition p) {
        switch (p) {
            case EdgePartition::Forward:
                return "forward";
            case EdgePartition::Keyed:
                return "keyed";
            case EdgePartition::Broadcast:
                return "broadcast";
        }
        return "unknown";
    }

    std::vector<LogicalOperator> operators_;
    std::vector<StreamEdge> edges_;
};

}  // namespace xtream
