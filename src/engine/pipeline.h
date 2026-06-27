#pragma once

#include <atomic>
#include <memory>
#include <optional>

#include "engine/compiler.h"
#include "graph/dataflow_graph.h"
#include "operators/physical/physical_operator.h"

namespace xtream {

class Pipeline {
public:
    Pipeline() = default;

    explicit Pipeline(DataflowGraph graph) : graph_(std::move(graph)) {
        root_ = Compiler::compile(*graph_);
    }

    explicit Pipeline(std::shared_ptr<PhysicalOperator> root) : root_(std::move(root)) {}

    Pipeline(Pipeline&& other) noexcept
        : graph_(std::move(other.graph_)),
          root_(std::move(other.root_)),
          running_(other.running_.load()) {}

    Pipeline& operator=(Pipeline&& other) noexcept {
        if (this != &other) {
            graph_ = std::move(other.graph_);
            root_ = std::move(other.root_);
            running_ = other.running_.load();
        }
        return *this;
    }

    void run(size_t event_count) {
        walk([](PhysicalOperator& op) { op.setup(); });
        walk([](PhysicalOperator& op) { op.open(); });

        for (size_t i = 0; i < event_count; ++i) {
            StreamElement dummy{Event<Record>(Record(nullptr), u64(0))};
            root_->execute(dummy);
        }

        walk([](PhysicalOperator& op) { op.close(); });
        walk([](PhysicalOperator& op) { op.terminate(); });
    }

    void run() {
        running_ = true;
        walk([](PhysicalOperator& op) { op.setup(); });
        walk([](PhysicalOperator& op) { op.open(); });

        while (running_) {
            StreamElement dummy{Event<Record>(Record(nullptr), u64(0))};
            root_->execute(dummy);
            if (root_->is_done()) {
                break;
            }
        }

        walk([](PhysicalOperator& op) { op.close(); });
        walk([](PhysicalOperator& op) { op.terminate(); });
    }

    void stop() { running_ = false; }

    std::shared_ptr<PhysicalOperator> root() const { return root_; }

private:
    template <typename Fn>
    void walk(Fn&& fn) {
        auto node = root_;
        while (node) {
            fn(*node);
            node = node->next();
        }
    }

    std::optional<DataflowGraph> graph_;
    std::shared_ptr<PhysicalOperator> root_;
    std::atomic<bool> running_{false};
};

}  // namespace xtream
