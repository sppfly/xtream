#pragma once

#include <cassert>
#include <cstdint>
#include <deque>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "core/types/window_spec.h"
#include "graph/dataflow_graph.h"
#include "graph/stream_edge.h"
#include "operators/logical/filter_logical_operator.h"
#include "operators/logical/interval_join_logical_operator.h"
#include "operators/logical/logical_operator.h"
#include "operators/logical/map_logical_operator.h"
#include "operators/logical/sink_logical_operator.h"
#include "operators/logical/source_logical_operator.h"
#include "operators/logical/window_join_logical_operator.h"
#include "operators/logical/window_logical_operator.h"

namespace xtream {

class DataflowGraphBuilder;

class StreamHandle {
public:
    StreamHandle(DataflowGraphBuilder& builder, OperatorId id) : builder_(builder), id_(id) {}

    StreamHandle map(MapLogicalOperator::Func func);
    StreamHandle filter(FilterLogicalOperator::Func func);

    StreamHandle window_join(const StreamHandle& other,
                             WindowJoinLogicalOperator::KeySelector left_key,
                             WindowJoinLogicalOperator::KeySelector right_key, WindowSpec spec,
                             WindowJoinLogicalOperator::JoinFunc func);
    StreamHandle interval_join(const StreamHandle& other,
                               IntervalJoinLogicalOperator::KeySelector left_key,
                               IntervalJoinLogicalOperator::KeySelector right_key, i64 lower_bound,
                               i64 upper_bound, IntervalJoinLogicalOperator::JoinFunc func);

    class WindowedStreamHandle {
    public:
        WindowedStreamHandle(DataflowGraphBuilder& builder, OperatorId upstream, WindowSpec spec)
            : builder_(builder), upstream_(upstream), spec_(spec) {}

        StreamHandle reduce(WindowLogicalOperator::Func func);
        StreamHandle aggregate(WindowLogicalOperator::Func func);

    private:
        DataflowGraphBuilder& builder_;
        OperatorId upstream_;
        WindowSpec spec_;
    };

    WindowedStreamHandle window(WindowSpec spec) {
        return WindowedStreamHandle(builder_, id_, spec);
    }

    StreamHandle with_watermark(u64 allowed_lateness);

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
    friend class StreamHandle::WindowedStreamHandle;

public:
    DataflowGraphBuilder() = default;

    StreamHandle source(SourceLogicalOperator::Func func) {
        auto id = next_id();
        nodes_.push_back(StreamNode{id, "source", u64(1), SourceLogicalOperator(std::move(func))});
        return StreamHandle(*this, id);
    }

    DataflowGraph build() const {
        auto result = validate();
        assert(result.is_ok());
        std::vector<LogicalOperator> ops;
        ops.reserve(nodes_.size());
        for (const auto& node : nodes_) {
            ops.push_back(std::visit(
                [&](const auto& op) -> LogicalOperator {
                    return LogicalOperator(node.id, node.name, node.parallelism, op);
                },
                node.op));
        }
        return DataflowGraph(ops, edges_);
    }

    ValidationResult validate() const {
        if (nodes_.empty()) {
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
    struct StreamNode {
        OperatorId id;
        std::string name;
        u64 parallelism;
        std::variant<SourceLogicalOperator, MapLogicalOperator, FilterLogicalOperator,
                     SinkLogicalOperator, WindowLogicalOperator, WindowJoinLogicalOperator,
                     IntervalJoinLogicalOperator>
            op;
    };

    OperatorId next_id() { return OperatorId(u64(counter_++)); }

    void add_node(StreamNode node) { nodes_.push_back(std::move(node)); }

    void add_edge(OperatorId from, OperatorId to, EdgePartition partition) {
        auto id = EdgeId(u64(edge_counter_++));
        edges_.emplace_back(id, from, to, partition);
    }

    void set_source_lateness(OperatorId id, u64 lateness) {
        for (auto& node : nodes_) {
            if (node.id != id) continue;
            auto* src = std::get_if<SourceLogicalOperator>(&node.op);
            assert(src && "with_watermark can only be applied to a source");
            src->set_allowed_lateness(lateness);
            return;
        }
        assert(false && "with_watermark: source id not found");
    }

    ValidationResult check_no_cycles() const {
        std::unordered_map<OperatorId, std::vector<OperatorId>> adjacency;
        for (const auto& node : nodes_) {
            adjacency[node.id] = {};
        }
        for (const auto& edge : edges_) {
            adjacency[edge.source()].push_back(edge.target());
        }

        std::unordered_map<OperatorId, int> in_degree;
        for (const auto& node : nodes_) {
            in_degree[node.id] = 0;
        }
        for (const auto& edge : edges_) {
            in_degree[edge.target()]++;
        }

        std::deque<OperatorId> queue;
        for (const auto& node : nodes_) {
            if (in_degree[node.id] == 0) {
                queue.push_back(node.id);
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

        if (visited != nodes_.size()) {
            return ValidationResult::error("graph contains a cycle");
        }
        return ValidationResult::ok();
    }

    ValidationResult check_edges() const {
        std::unordered_set<OperatorId> known_ids;
        for (const auto& node : nodes_) {
            known_ids.insert(node.id);
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
    std::vector<StreamNode> nodes_;
    std::vector<StreamEdge> edges_;
};

inline StreamHandle StreamHandle::map(MapLogicalOperator::Func func) {
    auto id = builder_.next_id();
    builder_.add_node(
        DataflowGraphBuilder::StreamNode{id, "map", u64(1), MapLogicalOperator(std::move(func))});
    builder_.add_edge(id_, id, EdgePartition::Forward);
    return StreamHandle(builder_, id);
}

inline StreamHandle StreamHandle::filter(FilterLogicalOperator::Func func) {
    auto id = builder_.next_id();
    builder_.add_node(DataflowGraphBuilder::StreamNode{id, "filter", u64(1),
                                                       FilterLogicalOperator(std::move(func))});
    builder_.add_edge(id_, id, EdgePartition::Forward);
    return StreamHandle(builder_, id);
}

inline StreamHandle StreamHandle::sink(SinkLogicalOperator::Func func) {
    auto id = builder_.next_id();
    builder_.add_node(
        DataflowGraphBuilder::StreamNode{id, "sink", u64(1), SinkLogicalOperator(std::move(func))});
    builder_.add_edge(id_, id, EdgePartition::Forward);
    return StreamHandle(builder_, id);
}

inline StreamHandle StreamHandle::WindowedStreamHandle::reduce(WindowLogicalOperator::Func func) {
    auto id = builder_.next_id();
    builder_.add_node(DataflowGraphBuilder::StreamNode{
        id, "window", u64(1), WindowLogicalOperator(spec_, std::move(func))});
    builder_.add_edge(upstream_, id, EdgePartition::Forward);
    return StreamHandle(builder_, id);
}

inline StreamHandle StreamHandle::WindowedStreamHandle::aggregate(
    WindowLogicalOperator::Func func) {
    auto id = builder_.next_id();
    builder_.add_node(DataflowGraphBuilder::StreamNode{
        id, "window", u64(1), WindowLogicalOperator(spec_, std::move(func))});
    builder_.add_edge(upstream_, id, EdgePartition::Forward);
    return StreamHandle(builder_, id);
}

inline StreamHandle StreamHandle::window_join(const StreamHandle& other,
                                              WindowJoinLogicalOperator::KeySelector left_key,
                                              WindowJoinLogicalOperator::KeySelector right_key,
                                              WindowSpec spec,
                                              WindowJoinLogicalOperator::JoinFunc func) {
    auto id = builder_.next_id();
    builder_.add_node(DataflowGraphBuilder::StreamNode{
        id, "window_join", u64(1),
        WindowJoinLogicalOperator(std::move(left_key), std::move(right_key), spec,
                                  std::move(func))});
    builder_.add_edge(id_, id, EdgePartition::Forward);
    builder_.add_edge(other.id_, id, EdgePartition::Forward);
    return StreamHandle(builder_, id);
}

inline StreamHandle StreamHandle::interval_join(const StreamHandle& other,
                                                IntervalJoinLogicalOperator::KeySelector left_key,
                                                IntervalJoinLogicalOperator::KeySelector right_key,
                                                i64 lower_bound, i64 upper_bound,
                                                IntervalJoinLogicalOperator::JoinFunc func) {
    auto id = builder_.next_id();
    builder_.add_node(DataflowGraphBuilder::StreamNode{
        id, "interval_join", u64(1),
        IntervalJoinLogicalOperator(std::move(left_key), std::move(right_key), lower_bound,
                                    upper_bound, std::move(func))});
    builder_.add_edge(id_, id, EdgePartition::Forward);
    builder_.add_edge(other.id_, id, EdgePartition::Forward);
    return StreamHandle(builder_, id);
}

inline DataflowGraph StreamHandle::build() {
    return builder_.build();
}

inline StreamHandle StreamHandle::with_watermark(u64 allowed_lateness) {
    builder_.set_source_lateness(id_, allowed_lateness);
    return *this;
}

}  // namespace xtream
