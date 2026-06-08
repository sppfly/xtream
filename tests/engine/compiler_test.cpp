#include "engine/compiler.h"

#include <gtest/gtest.h>

#include "graph/dataflow_graph.h"
#include "graph/dataflow_graph_builder.h"
#include "operators/logical/filter_logical_operator.h"
#include "operators/logical/logical_operator.h"
#include "operators/logical/map_logical_operator.h"
#include "operators/logical/sink_logical_operator.h"
#include "operators/logical/source_logical_operator.h"
#include "operators/physical/filter_physical_operator.h"
#include "operators/physical/map_physical_operator.h"
#include "operators/physical/sink_physical_operator.h"
#include "operators/physical/source_physical_operator.h"

namespace xtream {
namespace {

auto make_record(int v) -> Record {
    auto schema = std::make_shared<StreamSchema>();
    schema->add_field("value", TypeKind::I32);
    auto record = Record(schema);
    record.add_field(FieldValue::make_i32(i32(v)));
    return record;
}

auto make_event(int v, u64 ts = u64(0)) -> Event<Record> {
    return Event<Record>(make_record(v), ts);
}

auto make_value_extractor() {
    return [](const Event<Record>& event) -> i32 { return event.payload().field(0).as_i32(); };
}

TEST(CompileTest, MapLogicalCompilesToMapPhysical) {
    auto logical =
        LogicalOperator(OperatorId(u64(1)), "map", u64(1),
                        MapLogicalOperator([](Event<Record>& e) -> Event<Record> { return e; }));
    auto physical = logical.compile();
    ASSERT_NE(physical, nullptr);
    ASSERT_NE(dynamic_cast<MapPhysicalOperator*>(physical.get()), nullptr);
}

TEST(CompileTest, FilterLogicalCompilesToFilterPhysical) {
    auto logical = LogicalOperator(OperatorId(u64(1)), "filter", u64(1),
                                   FilterLogicalOperator([](Event<Record>&) { return true; }));
    auto physical = logical.compile();
    ASSERT_NE(dynamic_cast<FilterPhysicalOperator*>(physical.get()), nullptr);
}

TEST(CompileTest, SourceLogicalCompilesToSourcePhysical) {
    auto logical =
        LogicalOperator(OperatorId(u64(1)), "src", u64(1),
                        SourceLogicalOperator([]() -> Event<Record> { return make_event(0); }));
    auto physical = logical.compile();
    ASSERT_NE(dynamic_cast<SourcePhysicalOperator*>(physical.get()), nullptr);
}

TEST(CompileTest, SinkLogicalCompilesToSinkPhysical) {
    auto logical = LogicalOperator(OperatorId(u64(1)), "sink", u64(1),
                                   SinkLogicalOperator([](Event<Record>&) {}));
    auto physical = logical.compile();
    ASSERT_NE(dynamic_cast<SinkPhysicalOperator*>(physical.get()), nullptr);
}

TEST(CompilerTest, CompileLinearGraph) {
    auto graph = DataflowGraphBuilder()
                     .source([]() -> Event<Record> { return make_event(0); })
                     .map([](Event<Record>& e) -> Event<Record> {
                         auto val = make_value_extractor()(e);
                         return make_event(val.raw() + 1);
                     })
                     .sink([](Event<Record>&) {})
                     .build();

    auto root = Compiler::compile(graph);
    ASSERT_NE(root, nullptr);
    ASSERT_NE(dynamic_cast<SourcePhysicalOperator*>(root.get()), nullptr);
    ASSERT_NE(root->next(), nullptr);
}

TEST(CompilerTest, ExecuteCompiledChain) {
    std::vector<Event<Record>> data = {make_event(10), make_event(20), make_event(30)};
    size_t idx = 0;

    std::vector<Event<Record>> collected;

    auto graph = DataflowGraphBuilder()
                     .source([&data, &idx]() -> Event<Record> { return data[idx++]; })
                     .map([](Event<Record>& e) -> Event<Record> {
                         auto val = make_value_extractor()(e);
                         return make_event(val.raw() * 2);
                     })
                     .sink([&collected](Event<Record>& e) { collected.push_back(e); })
                     .build();

    auto root = Compiler::compile(graph);
    auto source = dynamic_cast<SourcePhysicalOperator*>(root.get());
    ASSERT_NE(source, nullptr);

    source->open();
    for (size_t i = 0; i < data.size(); ++i) {
        auto dummy = make_event(0);
        source->execute(dummy);
    }
    source->close();

    ASSERT_EQ(collected.size(), 3u);
    EXPECT_EQ(make_value_extractor()(collected[0]).raw(), 20);
    EXPECT_EQ(make_value_extractor()(collected[1]).raw(), 40);
    EXPECT_EQ(make_value_extractor()(collected[2]).raw(), 60);
}

TEST(CompilerTest, ExecuteWithFilter) {
    std::vector<Event<Record>> data = {make_event(5), make_event(15), make_event(25)};
    size_t idx = 0;

    std::vector<Event<Record>> collected;

    auto graph =
        DataflowGraphBuilder()
            .source([&data, &idx]() -> Event<Record> { return data[idx++]; })
            .filter([](Event<Record>& e) -> bool { return make_value_extractor()(e).raw() > 10; })
            .sink([&collected](Event<Record>& e) { collected.push_back(e); })
            .build();

    auto root = Compiler::compile(graph);
    auto source = dynamic_cast<SourcePhysicalOperator*>(root.get());

    source->open();
    for (size_t i = 0; i < data.size(); ++i) {
        auto dummy = make_event(0);
        source->execute(dummy);
    }
    source->close();

    ASSERT_EQ(collected.size(), 2u);
    EXPECT_EQ(make_value_extractor()(collected[0]).raw(), 15);
    EXPECT_EQ(make_value_extractor()(collected[1]).raw(), 25);
}

}  // namespace
}  // namespace xtream
