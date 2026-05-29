#pragma once

#include <cassert>
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

    const OperatorDescriptor& op(OperatorId id) const {
        for (const auto& op : operators_) {
            if (op.id() == id) {
                return op;
            }
        }
        assert(false);
    }

    const OperatorDescriptor& op(std::string_view name) const {
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

private:
    std::vector<OperatorDescriptor> operators_;
    std::vector<StreamEdge> edges_;
};

}  // namespace extream
