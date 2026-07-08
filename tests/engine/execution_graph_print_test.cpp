#include <gtest/gtest.h>

#include <string>

#include "core/types/window_spec.h"
#include "engine/execution_graph.h"
#include "graph/dataflow_graph_builder.h"
#include "operators/logical/sink_logical_operator.h"
#include "operators/logical/source_logical_operator.h"
#include "operators/logical/window_logical_operator.h"

namespace xtream {
namespace {

auto noop_source() -> Event<Record> {
    return Event<Record>(Record(nullptr), u64(0));
}
auto noop_sink(Event<Record>&) {}
auto noop_window(const std::vector<Event<Record>>&) -> Event<Record> {
    return Event<Record>(Record(nullptr), u64(0));
}

TEST(PhysicalGraphPrintTest, GraphvizSimpleChain) {
    auto graph = DataflowGraphBuilder()
                     .source(noop_source)
                     .map([](Event<Record>& e) { return e; })
                     .filter([](Event<Record>&) { return true; })
                     .sink(noop_sink)
                     .build();

    auto pipelines = ExecutionGraph::build(graph);
    auto dot = ExecutionGraph::to_graphviz(pipelines);

    EXPECT_NE(dot.find("digraph ExecutionGraph"), std::string::npos);
    EXPECT_NE(dot.find("Source"), std::string::npos);
    EXPECT_NE(dot.find("Map"), std::string::npos);
    EXPECT_NE(dot.find("Filter"), std::string::npos);
    EXPECT_NE(dot.find("Sink"), std::string::npos);
    // Single pipeline, no channel edges (no dashed edges)
    EXPECT_EQ(dot.find("style=dashed, label=\"channel"), std::string::npos);
}

TEST(PhysicalGraphPrintTest, GraphvizWithWindowSplitsPipelines) {
    auto graph = DataflowGraphBuilder()
                     .source(noop_source)
                     .window(time_tumbling_window(10_usize))
                     .reduce(noop_window)
                     .sink(noop_sink)
                     .build();

    auto pipelines = ExecutionGraph::build(graph);
    auto dot = ExecutionGraph::to_graphviz(pipelines);

    // Two pipelines with a channel edge
    EXPECT_NE(dot.find("cluster_p0"), std::string::npos);
    EXPECT_NE(dot.find("cluster_p1"), std::string::npos);
    EXPECT_NE(dot.find("TimeWindow"), std::string::npos);
    EXPECT_NE(dot.find("ChannelSource"), std::string::npos);
    EXPECT_NE(dot.find("style=dashed, label=\"channel"), std::string::npos);
}

TEST(PhysicalGraphPrintTest, JsonSimpleChain) {
    auto graph = DataflowGraphBuilder().source(noop_source).sink(noop_sink).build();

    auto pipelines = ExecutionGraph::build(graph);
    auto json = ExecutionGraph::to_json(pipelines);

    EXPECT_NE(json.find("\"pipelines\""), std::string::npos);
    EXPECT_NE(json.find("\"channels\""), std::string::npos);
    EXPECT_NE(json.find("\"type\": \"Source\""), std::string::npos);
    EXPECT_NE(json.find("\"type\": \"Sink\""), std::string::npos);
    // No channels: array is empty (no "from"/"to" entries)
    EXPECT_EQ(json.find("\"from\""), std::string::npos);
}

TEST(PhysicalGraphPrintTest, JsonWithWindowHasChannel) {
    auto graph = DataflowGraphBuilder()
                     .source(noop_source)
                     .window(time_tumbling_window(10_usize))
                     .reduce(noop_window)
                     .sink(noop_sink)
                     .build();

    auto pipelines = ExecutionGraph::build(graph);
    auto json = ExecutionGraph::to_json(pipelines);

    EXPECT_NE(json.find("\"channels\""), std::string::npos);
    EXPECT_NE(json.find("\"from\""), std::string::npos);
    EXPECT_NE(json.find("\"to\""), std::string::npos);
    // Channel 0 should exist
    EXPECT_NE(json.find("\"id\": 0"), std::string::npos);
}

}  // namespace
}  // namespace xtream
