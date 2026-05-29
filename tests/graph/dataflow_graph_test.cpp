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

TEST(DataflowGraphTest, FindOperatorById) {
    DataflowGraphBuilder builder;
    auto op_id = builder.add_operator("myop", "Map", 3);
    auto graph = builder.build();

    auto found = graph.find_operator(op_id);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->id(), op_id);
    EXPECT_EQ(found->name(), "myop");
    EXPECT_EQ(found->parallelism(), 3u);
}

TEST(DataflowGraphTest, FindOperatorByName) {
    DataflowGraphBuilder builder;
    builder.add_operator("alpha", "Op", 1);
    builder.add_operator("beta", "Op", 1);
    auto graph = builder.build();

    EXPECT_NE(graph.find_operator("alpha"), nullptr);
    EXPECT_NE(graph.find_operator("beta"), nullptr);
    EXPECT_EQ(graph.find_operator("gamma"), nullptr);
}

TEST(DataflowGraphTest, FindOperatorNotFound) {
    DataflowGraphBuilder builder;
    builder.add_operator("a", "Op", 1);
    auto graph = builder.build();

    OperatorId nonexistent(u64(999));
    EXPECT_EQ(graph.find_operator(nonexistent), nullptr);
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

TEST(DataflowGraphTest, SourcesOfReturnsCorrectOperators) {
    DataflowGraphBuilder builder;
    auto s1 = builder.add_operator("s1", "Source", 1);
    auto s2 = builder.add_operator("s2", "Source", 1);
    auto t = builder.add_operator("t", "Sink", 1);

    builder.add_edge(s1, t, EdgePartition::Forward);
    builder.add_edge(s2, t, EdgePartition::Forward);

    auto graph = builder.build();
    auto sources = graph.sources_of(t);
    ASSERT_EQ(sources.size(), 2u);
}

TEST(DataflowGraphTest, TargetsOfReturnsCorrectOperators) {
    DataflowGraphBuilder builder;
    auto s = builder.add_operator("s", "Source", 1);
    auto t1 = builder.add_operator("t1", "Sink", 1);
    auto t2 = builder.add_operator("t2", "Sink", 1);

    builder.add_edge(s, t1, EdgePartition::Forward);
    builder.add_edge(s, t2, EdgePartition::Keyed);

    auto graph = builder.build();
    auto targets = graph.targets_of(s);
    ASSERT_EQ(targets.size(), 2u);
}

TEST(DataflowGraphTest, NoEdgesProducesEmptySourceTargets) {
    DataflowGraphBuilder builder;
    builder.add_operator("a", "Op", 1);
    builder.add_operator("b", "Op", 1);
    auto graph = builder.build();

    auto sources = graph.sources_of(graph.operators()[1].id());
    EXPECT_TRUE(sources.empty());

    auto targets = graph.targets_of(graph.operators()[0].id());
    EXPECT_TRUE(targets.empty());
}

}  // namespace
}  // namespace extream
