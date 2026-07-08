#include <gtest/gtest.h>

#include <string>

#include "graph/dataflow_graph_builder.h"
#include "operators/logical/sink_logical_operator.h"
#include "operators/logical/source_logical_operator.h"
#include "operators/logical/window_join_logical_operator.h"

namespace xtream {
namespace {

auto noop_source() -> Event<Record> {
    return Event<Record>(Record(nullptr), u64(0));
}
auto noop_sink(Event<Record>&) {}
auto noop_key(const Event<Record>&) -> u64 {
    return u64(0);
}
auto noop_join(const Event<Record>&, const Event<Record>&) -> Event<Record> {
    return Event<Record>(Record(nullptr), u64(0));
}

TEST(GraphPrintTest, GraphvizContainsAllOperatorsAndEdges) {
    DataflowGraphBuilder builder;
    auto left = builder.source(noop_source);
    auto right = builder.source(noop_source);
    auto joined =
        left.window_join(right, noop_key, noop_key, time_tumbling_window(10_usize), noop_join);
    joined.sink(noop_sink);
    auto graph = builder.build();

    auto dot = graph.to_graphviz();

    EXPECT_NE(dot.find("digraph DataflowGraph"), std::string::npos);
    EXPECT_NE(dot.find("source"), std::string::npos);
    EXPECT_NE(dot.find("window_join"), std::string::npos);
    EXPECT_NE(dot.find("sink"), std::string::npos);
    EXPECT_NE(dot.find("WindowJoin"), std::string::npos);
    // 4 nodes, 3 edges: "->" should appear 3 times
    size_t arrow_count = 0;
    size_t pos = 0;
    while ((pos = dot.find("->", pos)) != std::string::npos) {
        arrow_count++;
        pos += 2;
    }
    EXPECT_EQ(arrow_count, 3u);
}

TEST(GraphPrintTest, GraphvizContainsPartitionLabel) {
    auto graph = DataflowGraphBuilder()
                     .source(noop_source)
                     .map([](Event<Record>& e) { return e; })
                     .sink(noop_sink)
                     .build();

    auto dot = graph.to_graphviz();
    EXPECT_NE(dot.find("forward"), std::string::npos);
}

TEST(GraphPrintTest, JsonIsValidStructure) {
    DataflowGraphBuilder builder;
    auto left = builder.source(noop_source);
    auto right = builder.source(noop_source);
    auto joined =
        left.window_join(right, noop_key, noop_key, time_tumbling_window(10_usize), noop_join);
    joined.sink(noop_sink);
    auto graph = builder.build();

    auto json = graph.to_json();

    EXPECT_NE(json.find("\"operators\""), std::string::npos);
    EXPECT_NE(json.find("\"edges\""), std::string::npos);
    EXPECT_NE(json.find("\"type\": \"WindowJoin\""), std::string::npos);
    EXPECT_NE(json.find("\"type\": \"Source\""), std::string::npos);
    EXPECT_NE(json.find("\"partition\": \"forward\""), std::string::npos);
    // 4 operators, 3 edges
    size_t source_count = 0;
    size_t pos = 0;
    while ((pos = json.find("\"type\": \"Source\"", pos)) != std::string::npos) {
        source_count++;
        pos += 16;
    }
    EXPECT_EQ(source_count, 2u);
}

TEST(GraphPrintTest, JsonAndGraphvizIncludeParallelism) {
    auto graph = DataflowGraphBuilder().source(noop_source).sink(noop_sink).build();

    auto dot = graph.to_graphviz();
    auto json = graph.to_json();
    EXPECT_NE(dot.find("p=1"), std::string::npos);
    EXPECT_NE(json.find("\"parallelism\": 1"), std::string::npos);
}

}  // namespace
}  // namespace xtream
