#include "operators/logical/logical_operator.h"

#include <string>

#include "gtest/gtest.h"
#include "operators/logical/filter_logical_operator.h"
#include "operators/logical/map_logical_operator.h"
#include "operators/logical/sink_logical_operator.h"
#include "operators/logical/source_logical_operator.h"

namespace extream {
namespace {

TEST(LogicalOperatorTest, SourceTypeName) {
    auto op = LogicalOperator(OperatorId(u64(1)), "my_source", u64(1),
                              SourceLogicalOperator([]() -> Event<Record> {
                                  return Event<Record>(Record(nullptr), i64(0));
                              }));
    EXPECT_EQ(op.name(), "my_source");
    EXPECT_EQ(op.parallelism(), u64(1));
    EXPECT_EQ(op.type_name(), "Source");
}

TEST(LogicalOperatorTest, SinkTypeName) {
    auto op = LogicalOperator(OperatorId(u64(2)), "my_sink", u64(2),
                              SinkLogicalOperator([](Event<Record>&) {}));
    EXPECT_EQ(op.name(), "my_sink");
    EXPECT_EQ(op.parallelism(), u64(2));
    EXPECT_EQ(op.type_name(), "Sink");
}

TEST(LogicalOperatorTest, MapTypeName) {
    auto lambda = [](Event<Record>& e) -> Event<Record> {
        return Event<Record>(e.payload(), i64(e.timestamp().raw() + 1));
    };
    auto op = LogicalOperator(OperatorId(u64(3)), "mapper", u64(3), MapLogicalOperator(lambda));
    EXPECT_EQ(op.name(), "mapper");
    EXPECT_EQ(op.parallelism(), u64(3));
    EXPECT_EQ(op.type_name(), "Map");
}

TEST(LogicalOperatorTest, FilterTypeName) {
    auto lambda = [](Event<Record>& e) -> bool { return e.timestamp() > i64(0); };
    auto op = LogicalOperator(OperatorId(u64(4)), "filter", u64(1), FilterLogicalOperator(lambda));
    EXPECT_EQ(op.name(), "filter");
    EXPECT_EQ(op.type_name(), "Filter");
}

TEST(LogicalOperatorTest, StoredInVector) {
    std::vector<LogicalOperator> ops;
    ops.emplace_back(OperatorId(u64(1)), "src", u64(1),
                     SourceLogicalOperator(
                         []() -> Event<Record> { return Event<Record>(Record(nullptr), i64(0)); }));
    ops.emplace_back(OperatorId(u64(2)), "map", u64(2), MapLogicalOperator([](Event<Record>& e) {
                         return Event<Record>(e.payload(), e.timestamp());
                     }));
    ops.emplace_back(OperatorId(u64(3)), "sink", u64(1),
                     SinkLogicalOperator([](Event<Record>&) {}));

    ASSERT_EQ(ops.size(), 3u);
    EXPECT_EQ(ops[0].type_name(), "Source");
    EXPECT_EQ(ops[1].type_name(), "Map");
    EXPECT_EQ(ops[2].type_name(), "Sink");
}

TEST(LogicalOperatorTest, IdRoundTrip) {
    auto id = OperatorId(u64(42));
    auto op = LogicalOperator(id, "myop", u64(1), SourceLogicalOperator([]() -> Event<Record> {
                                  return Event<Record>(Record(nullptr), i64(0));
                              }));
    EXPECT_EQ(op.id(), id);
}

}  // namespace
}  // namespace extream
