#pragma once

#include <cassert>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "graph/dataflow_graph.h"
#include "graph/operator_descriptor.h"
#include "graph/stream_edge.h"

namespace extream {

class ValidationResult {
public:
    static ValidationResult ok() { return ValidationResult(true, ""); }
    static ValidationResult error(std::string msg) {
        return ValidationResult(false, std::move(msg));
    }

    bool is_ok() const { return ok_; }
    const std::string& message() const { return message_; }

    explicit operator bool() const { return ok_; }

private:
    ValidationResult(bool ok, std::string msg) : ok_(ok), message_(std::move(msg)) {}
    bool ok_;
    std::string message_;
};

class DataflowGraphBuilder {
public:
    DataflowGraphBuilder() : next_operator_id_(0), next_edge_id_(0) {}

    OperatorId add_operator(std::string name, std::string type, size_t parallelism) {
        auto id = OperatorId(u64(next_operator_id_++));
        operators_.emplace_back(id, std::move(name), std::move(type), parallelism);
        return id;
    }

    EdgeId add_edge(OperatorId source, OperatorId target, EdgePartition partition) {
        auto id = EdgeId(u64(next_edge_id_++));
        edges_.emplace_back(id, source, target, partition);
        return id;
    }

    ValidationResult validate() const {
        if (operators_.empty()) {
            return ValidationResult::error("graph has no operators");
        }
        auto cycle_result = check_no_cycles();
        if (!cycle_result.is_ok()) {
            return cycle_result;
        }
        auto edge_result = check_edges();
        if (!edge_result.is_ok()) {
            return edge_result;
        }
        return ValidationResult::ok();
    }

    DataflowGraph build() const {
        auto result = validate();
        assert(result.is_ok());
        return DataflowGraph(operators_, edges_);
    }

private:
    ValidationResult check_no_cycles() const {
        std::unordered_map<OperatorId, std::vector<OperatorId>> adjacency;
        for (const auto& op : operators_) {
            adjacency[op.id()] = {};
        }
        for (const auto& edge : edges_) {
            adjacency[edge.source()].push_back(edge.target());
        }

        std::unordered_map<OperatorId, int> in_degree;
        for (const auto& op : operators_) {
            in_degree[op.id()] = 0;
        }
        for (const auto& edge : edges_) {
            in_degree[edge.target()]++;
        }

        std::deque<OperatorId> queue;
        for (const auto& op : operators_) {
            if (in_degree[op.id()] == 0) {
                queue.push_back(op.id());
            }
        }

        size_t visited = 0;
        while (!queue.empty()) {
            auto id = queue.front();
            queue.pop_front();
            visited++;
            for (const auto& neighbor : adjacency[id]) {
                in_degree[neighbor]--;
                if (in_degree[neighbor] == 0) {
                    queue.push_back(neighbor);
                }
            }
        }

        if (visited != operators_.size()) {
            return ValidationResult::error("graph contains a cycle");
        }
        return ValidationResult::ok();
    }

    ValidationResult check_edges() const {
        std::unordered_set<OperatorId> known_ids;
        for (const auto& op : operators_) {
            known_ids.insert(op.id());
        }

        for (const auto& edge : edges_) {
            if (known_ids.find(edge.source()) == known_ids.end()) {
                return ValidationResult::error("edge references unknown source operator");
            }
            if (known_ids.find(edge.target()) == known_ids.end()) {
                return ValidationResult::error("edge references unknown target operator");
            }
        }
        return ValidationResult::ok();
    }

    uint64_t next_operator_id_;
    uint64_t next_edge_id_;
    std::vector<OperatorDescriptor> operators_;
    std::vector<StreamEdge> edges_;
};

}  // namespace extream
