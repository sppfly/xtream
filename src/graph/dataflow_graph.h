#pragma once

#include <optional>
#include <string_view>
#include <vector>

#include "graph/operator_descriptor.h"
#include "graph/stream_edge.h"

namespace extream {

class DataflowGraph {
public:
    DataflowGraph(std::vector<OperatorDescriptor> operators, std::vector<StreamEdge> edges)
        : operators_(std::move(operators)), edges_(std::move(edges)) {}

    const std::vector<OperatorDescriptor>& operators() const { return operators_; }
    const std::vector<StreamEdge>& edges() const { return edges_; }

    const OperatorDescriptor* find_operator(OperatorId id) const {
        for (const auto& op : operators_) {
            if (op.id() == id) {
                return &op;
            }
        }
        return nullptr;
    }

    const OperatorDescriptor* find_operator(std::string_view name) const {
        for (const auto& op : operators_) {
            if (op.name() == name) {
                return &op;
            }
        }
        return nullptr;
    }

    const StreamEdge* find_edge(EdgeId id) const {
        for (const auto& edge : edges_) {
            if (edge.id() == id) {
                return &edge;
            }
        }
        return nullptr;
    }

    std::vector<const OperatorDescriptor*> sources_of(OperatorId target) const {
        std::vector<const OperatorDescriptor*> result;
        for (const auto& edge : edges_) {
            if (edge.target() == target) {
                auto* op = find_operator(edge.source());
                if (op) {
                    result.push_back(op);
                }
            }
        }
        return result;
    }

    std::vector<const OperatorDescriptor*> targets_of(OperatorId source) const {
        std::vector<const OperatorDescriptor*> result;
        for (const auto& edge : edges_) {
            if (edge.source() == source) {
                auto* op = find_operator(edge.target());
                if (op) {
                    result.push_back(op);
                }
            }
        }
        return result;
    }

private:
    std::vector<OperatorDescriptor> operators_;
    std::vector<StreamEdge> edges_;
};

}  // namespace extream
