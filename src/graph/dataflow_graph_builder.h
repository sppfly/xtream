#pragma once

#include <cassert>
#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "graph/dataflow_graph.h"
#include "graph/stream_edge.h"
#include "operators/logical/filter_logical_operator.h"
#include "operators/logical/logical_operator.h"
#include "operators/logical/map_logical_operator.h"
#include "operators/logical/sink_logical_operator.h"
#include "operators/logical/source_logical_operator.h"

namespace xtream {

class DataflowGraphBuilder;

class StreamHandle {
public:
    StreamHandle(DataflowGraphBuilder& builder, OperatorId id) : builder_(builder), id_(id) {}

    StreamHandle map(MapLogicalOperator::Func func);
    StreamHandle filter(FilterLogicalOperator::Func func);
    StreamHandle sink(SinkLogicalOperator::Func func);

    DataflowGraph build();

    OperatorId id() const { return id_; }

private:
    DataflowGraphBuilder& builder_;
    OperatorId id_;
};

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
    friend class StreamHandle;

public:
    DataflowGraphBuilder() = default;

    StreamHandle source(SourceLogicalOperator::Func func) {
        auto id = next_id();
        operators_.emplace_back(id, "source", u64(1), SourceLogicalOperator(std::move(func)));
        return StreamHandle(*this, id);
    }

    DataflowGraph build() const {
        auto result = validate();
        assert(result.is_ok());
        return DataflowGraph(operators_, edges_);
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

private:
    OperatorId next_id() { return OperatorId(u64(counter_++)); }

    void add_operator(LogicalOperator op) { operators_.push_back(std::move(op)); }

    void add_edge(OperatorId from, OperatorId to, EdgePartition partition) {
        auto id = EdgeId(u64(edge_counter_++));
        edges_.emplace_back(id, from, to, partition);
    }

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

    uint64_t counter_ = 1;
    uint64_t edge_counter_ = 0;
    std::vector<LogicalOperator> operators_;
    std::vector<StreamEdge> edges_;
};

inline StreamHandle StreamHandle::map(MapLogicalOperator::Func func) {
    auto id = builder_.next_id();
    builder_.add_operator(LogicalOperator(id, "map", u64(1), MapLogicalOperator(std::move(func))));
    builder_.add_edge(id_, id, EdgePartition::Forward);
    return StreamHandle(builder_, id);
}

inline StreamHandle StreamHandle::filter(FilterLogicalOperator::Func func) {
    auto id = builder_.next_id();
    builder_.add_operator(
        LogicalOperator(id, "filter", u64(1), FilterLogicalOperator(std::move(func))));
    builder_.add_edge(id_, id, EdgePartition::Forward);
    return StreamHandle(builder_, id);
}

inline StreamHandle StreamHandle::sink(SinkLogicalOperator::Func func) {
    auto id = builder_.next_id();
    builder_.add_operator(
        LogicalOperator(id, "sink", u64(1), SinkLogicalOperator(std::move(func))));
    builder_.add_edge(id_, id, EdgePartition::Forward);
    return StreamHandle(builder_, id);
}

inline DataflowGraph StreamHandle::build() {
    return builder_.build();
}

}  // namespace xtream
