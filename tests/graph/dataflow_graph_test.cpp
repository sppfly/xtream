#include <gtest/gtest.h>

#include "graph/dataflow_graph_builder.h"
#include "operators/logical/sink_logical_operator.h"
#include "operators/logical/source_logical_operator.h"

namespace xtream {
namespace {

auto noop_source() -> Event<Record> {
    return Event<Record>(Record(nullptr), i64(0));
}
auto noop_sink(Event<Record>&) {}

TEST(DataflowGraphTest, EmptyGraphFailsValidation) {
    DataflowGraphBuilder builder;
    auto result = builder.validate();
    EXPECT_FALSE(result.is_ok());
}

TEST(DataflowGraphTest, SingleOperatorPassesValidation) {
    DataflowGraphBuilder builder;
    builder.source(noop_source);
    auto result = builder.validate();
    EXPECT_TRUE(result.is_ok());
}

TEST(DataflowGraphTest, BuildProducesGraph) {
    auto graph = DataflowGraphBuilder().source(noop_source).build();
    EXPECT_EQ(graph.operators().size(), 1u);
    EXPECT_EQ(graph.edges().size(), 0u);
}

TEST(DataflowGraphTest, SimpleLinearDAG) {
    auto graph = DataflowGraphBuilder()
                     .source(noop_source)
                     .map([](Event<Record>& e) { return e; })
                     .sink(noop_sink)
                     .build();
    EXPECT_EQ(graph.operators().size(), 3u);
    EXPECT_EQ(graph.edges().size(), 2u);
}

TEST(DataflowGraphTest, FanOutDAG) {
    DataflowGraphBuilder builder;
    auto src = builder.source(noop_source);
    src.map([](Event<Record>& e) { return e; });
    src.map([](Event<Record>& e) { return e; });

    auto result = builder.validate();
    EXPECT_TRUE(result.is_ok());

    auto graph = builder.build();
    auto targets = graph.targets_of(src.id());
    EXPECT_EQ(targets.size(), 2u);
}

TEST(DataflowGraphTest, FanInDAG) {
    DataflowGraphBuilder builder;
    auto s1 = builder.source(noop_source);
    auto s2 = builder.source(noop_source);
    auto result = builder.validate();
    EXPECT_TRUE(result.is_ok());

    auto graph = builder.build();
    EXPECT_EQ(graph.operators().size(), 2u);
}

TEST(DataflowGraphTest, OpById) {
    DataflowGraphBuilder builder;
    auto src = builder.source(noop_source);
    auto graph = builder.build();

    const auto& op = graph.op(src.id());
    EXPECT_EQ(op.id(), src.id());
}

TEST(DataflowGraphTest, OpByName) {
    auto graph = DataflowGraphBuilder().source(noop_source).build();
    EXPECT_EQ(graph.op("source").name(), "source");
}

TEST(DataflowGraphTest, EdgePartitionModes) {
    auto graph = DataflowGraphBuilder()
                     .source(noop_source)
                     .map([](Event<Record>& e) { return e; })
                     .sink(noop_sink)
                     .build();
    EXPECT_EQ(graph.edges()[0].partition(), EdgePartition::Forward);
    EXPECT_EQ(graph.edges()[1].partition(), EdgePartition::Forward);
}

TEST(DataflowGraphTest, SourcesOf) {
    auto graph = DataflowGraphBuilder().source(noop_source).sink(noop_sink).build();
    auto sources = graph.sources_of(graph.operators()[1].id());
    ASSERT_EQ(sources.size(), 1u);
    EXPECT_EQ(sources[0], graph.operators()[0].id());
}

TEST(DataflowGraphTest, TargetsOf) {
    auto graph = DataflowGraphBuilder().source(noop_source).sink(noop_sink).build();
    auto targets = graph.targets_of(graph.operators()[0].id());
    ASSERT_EQ(targets.size(), 1u);
    EXPECT_EQ(targets[0], graph.operators()[1].id());
}

TEST(DataflowGraphTest, NoEdgesProducesEmptySourceTargets) {
    DataflowGraphBuilder builder;
    builder.source(noop_source);
    auto graph = builder.build();

    EXPECT_TRUE(graph.sources_of(graph.operators()[0].id()).empty());
    EXPECT_TRUE(graph.targets_of(graph.operators()[0].id()).empty());
}

TEST(DataflowGraphTest, Roots) {
    auto graph = DataflowGraphBuilder()
                     .source(noop_source)
                     .map([](Event<Record>& e) { return e; })
                     .sink(noop_sink)
                     .build();
    auto roots = graph.roots();
    ASSERT_EQ(roots.size(), 1u);
    EXPECT_EQ(roots[0], graph.operators()[0].id());
}

TEST(DataflowGraphTest, MultipleRoots) {
    DataflowGraphBuilder builder;
    auto s1 = builder.source(noop_source);
    auto s2 = builder.source(noop_source);
    auto graph = builder.build();
    auto roots = graph.roots();
    ASSERT_EQ(roots.size(), 2u);
}

TEST(DataflowGraphTest, Leaves) {
    DataflowGraphBuilder builder;
    auto src = builder.source(noop_source);
    src.map([](Event<Record>& e) { return e; });
    src.map([](Event<Record>& e) { return e; });

    auto graph = builder.build();
    auto leaves = graph.leaves();
    ASSERT_EQ(leaves.size(), 2u);
}

TEST(DataflowGraphTest, InOutDegree) {
    auto graph = DataflowGraphBuilder()
                     .source(noop_source)
                     .map([](Event<Record>& e) { return e; })
                     .sink(noop_sink)
                     .build();
    EXPECT_EQ(graph.in_degree_of(graph.operators()[0].id()), 0u);
    EXPECT_EQ(graph.out_degree_of(graph.operators()[0].id()), 1u);
    EXPECT_EQ(graph.in_degree_of(graph.operators()[1].id()), 1u);
    EXPECT_EQ(graph.out_degree_of(graph.operators()[1].id()), 1u);
    EXPECT_EQ(graph.in_degree_of(graph.operators()[2].id()), 1u);
    EXPECT_EQ(graph.out_degree_of(graph.operators()[2].id()), 0u);
}

TEST(DataflowGraphTest, TopologicalOrderSimple) {
    auto graph = DataflowGraphBuilder()
                     .source(noop_source)
                     .map([](Event<Record>& e) { return e; })
                     .sink(noop_sink)
                     .build();
    auto order = graph.topological_order();
    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], graph.operators()[0].id());
    EXPECT_EQ(order[1], graph.operators()[1].id());
    EXPECT_EQ(order[2], graph.operators()[2].id());
}

TEST(DataflowGraphTest, TopologicalOrderDiamond) {
    DataflowGraphBuilder builder;
    auto src = builder.source(noop_source);
    auto branch1 = src.map([](Event<Record>& e) { return e; });
    auto branch2 = src.map([](Event<Record>& e) { return e; });
    auto graph = builder.build();
    auto order = graph.topological_order();
    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], src.id());
}

}  // namespace
}  // namespace xtream
