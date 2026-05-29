#include <gtest/gtest.h>

#include "graph/dataflow_graph_builder.h"

namespace extream {
namespace {

TEST(DataflowGraphTest, EmptyGraphFailsValidation) {
    DataflowGraphBuilder builder;
    auto result = builder.validate();
    EXPECT_FALSE(result.is_ok());
}

TEST(DataflowGraphTest, SingleOperatorPassesValidation) {
    DataflowGraphBuilder builder;
    builder.add_operator("src", "Source", 1);
    auto result = builder.validate();
    EXPECT_TRUE(result.is_ok());
}

TEST(DataflowGraphTest, BuildProducesGraph) {
    DataflowGraphBuilder builder;
    builder.add_operator("src", "Source", 1);
    builder.add_operator("sink", "Sink", 1);

    auto graph = builder.build();
    EXPECT_EQ(graph.operators().size(), 2u);
    EXPECT_EQ(graph.edges().size(), 0u);
}

TEST(DataflowGraphTest, SimpleLinearDAG) {
    DataflowGraphBuilder builder;
    auto src = builder.add_operator("src", "Source", 1);
    auto map = builder.add_operator("map", "Map", 2);
    auto sink = builder.add_operator("sink", "Sink", 1);

    builder.add_edge(src, map, EdgePartition::Forward);
    builder.add_edge(map, sink, EdgePartition::Forward);

    auto result = builder.validate();
    EXPECT_TRUE(result.is_ok()) << result.message();

    auto graph = builder.build();
    EXPECT_EQ(graph.operators().size(), 3u);
    EXPECT_EQ(graph.edges().size(), 2u);
}

TEST(DataflowGraphTest, FanOutDAG) {
    DataflowGraphBuilder builder;
    auto src = builder.add_operator("src", "Source", 1);
    auto op1 = builder.add_operator("op1", "Map", 1);
    auto op2 = builder.add_operator("op2", "Map", 1);

    builder.add_edge(src, op1, EdgePartition::Forward);
    builder.add_edge(src, op2, EdgePartition::Forward);

    auto result = builder.validate();
    EXPECT_TRUE(result.is_ok());

    auto graph = builder.build();
    auto targets = graph.targets_of(src);
    EXPECT_EQ(targets.size(), 2u);
    EXPECT_EQ(graph.op(targets[0]).name(), "op1");
    EXPECT_EQ(graph.op(targets[1]).name(), "op2");
}

TEST(DataflowGraphTest, FanInDAG) {
    DataflowGraphBuilder builder;
    auto src1 = builder.add_operator("src1", "Source", 1);
    auto src2 = builder.add_operator("src2", "Source", 1);
    auto join = builder.add_operator("join", "Join", 2);

    builder.add_edge(src1, join, EdgePartition::Forward);
    builder.add_edge(src2, join, EdgePartition::Forward);

    auto result = builder.validate();
    EXPECT_TRUE(result.is_ok());

    auto graph = builder.build();
    auto sources = graph.sources_of(join);
    EXPECT_EQ(sources.size(), 2u);
}

TEST(DataflowGraphTest, CycleDetected) {
    DataflowGraphBuilder builder;
    auto a = builder.add_operator("a", "Op", 1);
    auto b = builder.add_operator("b", "Op", 1);
    auto c = builder.add_operator("c", "Op", 1);

    builder.add_edge(a, b, EdgePartition::Forward);
    builder.add_edge(b, c, EdgePartition::Forward);
    builder.add_edge(c, a, EdgePartition::Forward);

    auto result = builder.validate();
    EXPECT_FALSE(result.is_ok());
    EXPECT_NE(result.message().find("cycle"), std::string::npos);
}

TEST(DataflowGraphTest, SelfLoopDetected) {
    DataflowGraphBuilder builder;
    auto a = builder.add_operator("a", "Op", 1);
    builder.add_edge(a, a, EdgePartition::Forward);

    auto result = builder.validate();
    EXPECT_FALSE(result.is_ok());
}

TEST(DataflowGraphTest, TwoNodeCycle) {
    DataflowGraphBuilder builder;
    auto a = builder.add_operator("a", "Op", 1);
    auto b = builder.add_operator("b", "Op", 1);

    builder.add_edge(a, b, EdgePartition::Forward);
    builder.add_edge(b, a, EdgePartition::Forward);

    auto result = builder.validate();
    EXPECT_FALSE(result.is_ok());
}

TEST(DataflowGraphTest, OpById) {
    DataflowGraphBuilder builder;
    auto op_id = builder.add_operator("myop", "Map", 3);
    auto graph = builder.build();

    const auto& op = graph.op(op_id);
    EXPECT_EQ(op.id(), op_id);
    EXPECT_EQ(op.name(), "myop");
    EXPECT_EQ(op.parallelism(), 3u);
}

TEST(DataflowGraphTest, OpByName) {
    DataflowGraphBuilder builder;
    builder.add_operator("alpha", "Op", 1);
    builder.add_operator("beta", "Op", 1);
    auto graph = builder.build();

    EXPECT_EQ(graph.op("alpha").name(), "alpha");
    EXPECT_EQ(graph.op("beta").name(), "beta");
}

TEST(DataflowGraphTest, EdgePartitionModes) {
    DataflowGraphBuilder builder;
    auto src = builder.add_operator("src", "Source", 1);
    auto t1 = builder.add_operator("t1", "Op", 1);
    auto t2 = builder.add_operator("t2", "Op", 1);
    auto t3 = builder.add_operator("t3", "Op", 1);

    builder.add_edge(src, t1, EdgePartition::Forward);
    builder.add_edge(src, t2, EdgePartition::Keyed);
    builder.add_edge(src, t3, EdgePartition::Broadcast);

    auto graph = builder.build();
    EXPECT_EQ(graph.edges()[0].partition(), EdgePartition::Forward);
    EXPECT_EQ(graph.edges()[1].partition(), EdgePartition::Keyed);
    EXPECT_EQ(graph.edges()[2].partition(), EdgePartition::Broadcast);
}

TEST(DataflowGraphTest, SourcesOf) {
    DataflowGraphBuilder builder;
    auto s1 = builder.add_operator("s1", "Source", 1);
    auto s2 = builder.add_operator("s2", "Source", 1);
    auto t = builder.add_operator("t", "Sink", 1);

    builder.add_edge(s1, t, EdgePartition::Forward);
    builder.add_edge(s2, t, EdgePartition::Forward);

    auto graph = builder.build();
    auto sources = graph.sources_of(t);
    ASSERT_EQ(sources.size(), 2u);
    EXPECT_EQ(sources[0], s1);
    EXPECT_EQ(sources[1], s2);
}

TEST(DataflowGraphTest, TargetsOf) {
    DataflowGraphBuilder builder;
    auto s = builder.add_operator("s", "Source", 1);
    auto t1 = builder.add_operator("t1", "Sink", 1);
    auto t2 = builder.add_operator("t2", "Sink", 1);

    builder.add_edge(s, t1, EdgePartition::Forward);
    builder.add_edge(s, t2, EdgePartition::Keyed);

    auto graph = builder.build();
    auto targets = graph.targets_of(s);
    ASSERT_EQ(targets.size(), 2u);
    EXPECT_EQ(targets[0], t1);
    EXPECT_EQ(targets[1], t2);
}

TEST(DataflowGraphTest, NoEdgesProducesEmptySourceTargets) {
    DataflowGraphBuilder builder;
    builder.add_operator("a", "Op", 1);
    builder.add_operator("b", "Op", 1);
    auto graph = builder.build();

    EXPECT_TRUE(graph.sources_of(graph.operators()[1].id()).empty());
    EXPECT_TRUE(graph.targets_of(graph.operators()[0].id()).empty());
}

TEST(DataflowGraphTest, Roots) {
    DataflowGraphBuilder builder;
    auto src = builder.add_operator("src", "Source", 1);
    auto map = builder.add_operator("map", "Map", 1);
    auto sink = builder.add_operator("sink", "Sink", 1);

    builder.add_edge(src, map, EdgePartition::Forward);
    builder.add_edge(map, sink, EdgePartition::Forward);

    auto graph = builder.build();
    auto roots = graph.roots();
    ASSERT_EQ(roots.size(), 1u);
    EXPECT_EQ(roots[0], src);
}

TEST(DataflowGraphTest, MultipleRoots) {
    DataflowGraphBuilder builder;
    auto s1 = builder.add_operator("s1", "Source", 1);
    auto s2 = builder.add_operator("s2", "Source", 1);
    auto join = builder.add_operator("join", "Join", 1);

    builder.add_edge(s1, join, EdgePartition::Forward);
    builder.add_edge(s2, join, EdgePartition::Forward);

    auto graph = builder.build();
    auto roots = graph.roots();
    ASSERT_EQ(roots.size(), 2u);
}

TEST(DataflowGraphTest, Leaves) {
    DataflowGraphBuilder builder;
    auto src = builder.add_operator("src", "Source", 1);
    auto op1 = builder.add_operator("op1", "Map", 1);
    auto op2 = builder.add_operator("op2", "Map", 1);

    builder.add_edge(src, op1, EdgePartition::Forward);
    builder.add_edge(src, op2, EdgePartition::Forward);

    auto graph = builder.build();
    auto leaves = graph.leaves();
    ASSERT_EQ(leaves.size(), 2u);
    EXPECT_EQ(leaves[0], op1);
    EXPECT_EQ(leaves[1], op2);
}

TEST(DataflowGraphTest, InOutDegree) {
    DataflowGraphBuilder builder;
    auto src = builder.add_operator("src", "Source", 1);
    auto map = builder.add_operator("map", "Map", 1);
    auto sink = builder.add_operator("sink", "Sink", 1);

    builder.add_edge(src, map, EdgePartition::Forward);
    builder.add_edge(map, sink, EdgePartition::Forward);

    auto graph = builder.build();
    EXPECT_EQ(graph.in_degree_of(src), 0u);
    EXPECT_EQ(graph.out_degree_of(src), 1u);
    EXPECT_EQ(graph.in_degree_of(map), 1u);
    EXPECT_EQ(graph.out_degree_of(map), 1u);
    EXPECT_EQ(graph.in_degree_of(sink), 1u);
    EXPECT_EQ(graph.out_degree_of(sink), 0u);
}

TEST(DataflowGraphTest, TopologicalOrderSimple) {
    DataflowGraphBuilder builder;
    auto src = builder.add_operator("src", "Source", 1);
    auto map = builder.add_operator("map", "Map", 1);
    auto sink = builder.add_operator("sink", "Sink", 1);

    builder.add_edge(src, map, EdgePartition::Forward);
    builder.add_edge(map, sink, EdgePartition::Forward);

    auto graph = builder.build();
    auto order = graph.topological_order();
    ASSERT_EQ(order.size(), 3u);
    EXPECT_EQ(order[0], src);
    EXPECT_EQ(order[1], map);
    EXPECT_EQ(order[2], sink);
}

TEST(DataflowGraphTest, TopologicalOrderDiamond) {
    DataflowGraphBuilder builder;
    auto src = builder.add_operator("src", "Source", 1);
    auto op1 = builder.add_operator("op1", "Map", 1);
    auto op2 = builder.add_operator("op2", "Map", 1);
    auto sink = builder.add_operator("sink", "Sink", 1);

    builder.add_edge(src, op1, EdgePartition::Forward);
    builder.add_edge(src, op2, EdgePartition::Forward);
    builder.add_edge(op1, sink, EdgePartition::Forward);
    builder.add_edge(op2, sink, EdgePartition::Forward);

    auto graph = builder.build();
    auto order = graph.topological_order();
    ASSERT_EQ(order.size(), 4u);

    EXPECT_EQ(order[0], src);
    EXPECT_EQ(order[3], sink);

    auto src_pos = size_t{0};
    auto sink_pos = order.size() - 1;
    for (size_t i = 0; i < order.size(); ++i) {
        if (order[i] == op1 || order[i] == op2) {
            EXPECT_GT(i, src_pos);
            EXPECT_LT(i, sink_pos);
        }
    }
}

}  // namespace
}  // namespace extream
