#include <gtest/gtest.h>

#include "graph/dataflow_graph_builder.h"
#include "operators/logical/interval_join_logical_operator.h"
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

TEST(JoinFluentApiTest, WindowJoinCreatesNodeAndTwoEdges) {
    DataflowGraphBuilder builder;
    auto left = builder.source(noop_source);
    auto right = builder.source(noop_source);
    auto joined =
        left.window_join(right, noop_key, noop_key, time_tumbling_window(10_usize), noop_join);
    joined.sink(noop_sink);

    auto result = builder.validate();
    ASSERT_TRUE(result.is_ok()) << result.message();

    auto graph = builder.build();
    EXPECT_EQ(graph.operators().size(), 4u);
    EXPECT_EQ(graph.edges().size(), 3u);

    bool found_join = false;
    for (const auto& op : graph.operators()) {
        if (op.type_name() == "WindowJoin") {
            found_join = true;
            break;
        }
    }
    EXPECT_TRUE(found_join);
}

TEST(JoinFluentApiTest, IntervalJoinCreatesNodeAndTwoEdges) {
    DataflowGraphBuilder builder;
    auto left = builder.source(noop_source);
    auto right = builder.source(noop_source);
    auto joined = left.interval_join(right, noop_key, noop_key, i64(-5), i64(10), noop_join);
    joined.sink(noop_sink);

    auto result = builder.validate();
    ASSERT_TRUE(result.is_ok()) << result.message();

    auto graph = builder.build();
    EXPECT_EQ(graph.operators().size(), 4u);
    EXPECT_EQ(graph.edges().size(), 3u);

    bool found_join = false;
    for (const auto& op : graph.operators()) {
        if (op.type_name() == "IntervalJoin") {
            found_join = true;
            break;
        }
    }
    EXPECT_TRUE(found_join);
}

TEST(JoinFluentApiTest, JoinWithUpstreamTransforms) {
    DataflowGraphBuilder builder;
    auto left = builder.source(noop_source).map([](Event<Record>& e) { return e; });
    auto right = builder.source(noop_source).filter([](Event<Record>&) { return true; });
    auto joined =
        left.window_join(right, noop_key, noop_key, time_tumbling_window(5_usize), noop_join);
    joined.sink(noop_sink);

    auto result = builder.validate();
    ASSERT_TRUE(result.is_ok()) << result.message();

    auto graph = builder.build();
    EXPECT_EQ(graph.operators().size(), 6u);
    EXPECT_EQ(graph.edges().size(), 5u);
}

TEST(JoinFluentApiTest, DiamondTopology) {
    DataflowGraphBuilder builder;
    auto src = builder.source(noop_source);
    auto left = src.map([](Event<Record>& e) { return e; });
    auto right = src.map([](Event<Record>& e) { return e; });
    auto joined =
        left.window_join(right, noop_key, noop_key, time_tumbling_window(10_usize), noop_join);
    joined.sink(noop_sink);

    auto result = builder.validate();
    ASSERT_TRUE(result.is_ok()) << result.message();

    auto graph = builder.build();
    EXPECT_EQ(graph.operators().size(), 5u);
    EXPECT_EQ(graph.edges().size(), 5u);
}

}  // namespace
}  // namespace xtream
