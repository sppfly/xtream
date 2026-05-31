#include "engine/compiler.h"

#include <gtest/gtest.h>

#include "graph/dataflow_graph.h"
#include "graph/dataflow_graph_builder.h"
#include "operators/logical/filter_logical_operator.h"
#include "operators/logical/logical_operator.h"
#include "operators/logical/map_logical_operator.h"
#include "operators/logical/sink_logical_operator.h"
#include "operators/logical/source_logical_operator.h"
#include "operators/physical/collect_sink_physical_operator.h"
#include "operators/physical/collection_source_physical_operator.h"
#include "operators/physical/filter_physical_operator.h"
#include "operators/physical/map_physical_operator.h"

namespace extream {
namespace {

auto make_record(int v) -> Record {
    auto schema = std::make_shared<StreamSchema>();
    schema->add_field("value", TypeKind::I32);
    auto record = Record(schema);
    record.add_field(FieldValue::make_i32(i32(v)));
    return record;
}

auto make_event(int v, i64 ts = i64(0)) -> Event<Record> {
    return Event<Record>(make_record(v), ts);
}

auto make_value_extractor() {
    return [](const Event<Record>& event) -> i32 { return event.payload().field(0).as_i32(); };
}

TEST(CompileTest, MapLogicalCompilesToMapPhysical) {
    auto logical = LogicalOperator(
        OperatorId(u64(1)), "map", u64(1),
        MapLogicalOperatorImpl([](Event<Record>& e) -> Event<Record> { return e; }));
    auto physical = logical.compile();
    ASSERT_NE(physical, nullptr);
    ASSERT_NE(dynamic_cast<MapPhysicalOperator*>(physical.get()), nullptr);
}

TEST(CompileTest, FilterLogicalCompilesToFilterPhysical) {
    auto logical = LogicalOperator(OperatorId(u64(1)), "filter", u64(1),
                                   FilterLogicalOperatorImpl([](Event<Record>&) { return true; }));
    auto physical = logical.compile();
    ASSERT_NE(dynamic_cast<FilterPhysicalOperator*>(physical.get()), nullptr);
}

TEST(CompileTest, SourceLogicalCompilesToSourcePhysical) {
    auto logical = LogicalOperator(OperatorId(u64(1)), "src", u64(1), SourceLogicalOperatorImpl());
    auto physical = logical.compile();
    ASSERT_NE(dynamic_cast<CollectionSourcePhysicalOperator*>(physical.get()), nullptr);
}

TEST(CompileTest, SinkLogicalCompilesToSinkPhysical) {
    auto logical = LogicalOperator(OperatorId(u64(1)), "sink", u64(1), SinkLogicalOperatorImpl());
    auto physical = logical.compile();
    ASSERT_NE(dynamic_cast<CollectSinkPhysicalOperator*>(physical.get()), nullptr);
}

TEST(CompilerTest, CompileLinearGraph) {
    DataflowGraphBuilder builder;
    auto src = builder.add_operator("src", u64(1), SourceLogicalOperatorImpl());
    auto map = builder.add_operator("map", u64(1),
                                    MapLogicalOperatorImpl([](Event<Record>& e) -> Event<Record> {
                                        auto val = make_value_extractor()(e);
                                        return make_event(val.raw() + 1);
                                    }));
    auto sink = builder.add_operator("sink", u64(1), SinkLogicalOperatorImpl());
    builder.add_edge(src, map, EdgePartition::Forward);
    builder.add_edge(map, sink, EdgePartition::Forward);
    auto graph = builder.build();

    auto root = Compiler::compile(graph);
    ASSERT_NE(root, nullptr);
    ASSERT_NE(dynamic_cast<CollectionSourcePhysicalOperator*>(root.get()), nullptr);
    ASSERT_NE(root->next(), nullptr);
}

TEST(CompilerTest, ExecuteCompiledChain) {
    SourceLogicalOperatorImpl::Data data = {make_event(10), make_event(20), make_event(30)};

    DataflowGraphBuilder builder;
    auto src_id = builder.add_operator("src", u64(1), SourceLogicalOperatorImpl(data));
    auto map_id = builder.add_operator(
        "map", u64(1), MapLogicalOperatorImpl([](Event<Record>& e) -> Event<Record> {
            auto val = make_value_extractor()(e);
            return make_event(val.raw() * 2);
        }));
    auto sink_id = builder.add_operator("sink", u64(1), SinkLogicalOperatorImpl());
    builder.add_edge(src_id, map_id, EdgePartition::Forward);
    builder.add_edge(map_id, sink_id, EdgePartition::Forward);
    auto graph = builder.build();

    auto root = Compiler::compile(graph);
    auto source = dynamic_cast<CollectionSourcePhysicalOperator*>(root.get());
    ASSERT_NE(source, nullptr);

    source->open();
    for (size_t i = 0; i < source->data().size(); ++i) {
        auto dummy = make_event(0);
        source->execute(dummy);
    }
    source->close();

    auto map_ptr = root->next();
    auto sink_ptr = map_ptr->next();
    auto sink = std::dynamic_pointer_cast<CollectSinkPhysicalOperator>(sink_ptr);
    ASSERT_NE(sink, nullptr);
    ASSERT_EQ(sink->collected().size(), 3u);
    EXPECT_EQ(make_value_extractor()(sink->collected()[0]).raw(), 20);
    EXPECT_EQ(make_value_extractor()(sink->collected()[1]).raw(), 40);
    EXPECT_EQ(make_value_extractor()(sink->collected()[2]).raw(), 60);
}

TEST(CompilerTest, ExecuteWithFilter) {
    SourceLogicalOperatorImpl::Data data = {make_event(5), make_event(15), make_event(25)};

    DataflowGraphBuilder builder;
    auto src_id = builder.add_operator("src", u64(1), SourceLogicalOperatorImpl(data));
    auto filter_id = builder.add_operator("filter", u64(1),
                                          FilterLogicalOperatorImpl([](Event<Record>& e) -> bool {
                                              return make_value_extractor()(e).raw() > 10;
                                          }));
    auto sink_id = builder.add_operator("sink", u64(1), SinkLogicalOperatorImpl());
    builder.add_edge(src_id, filter_id, EdgePartition::Forward);
    builder.add_edge(filter_id, sink_id, EdgePartition::Forward);
    auto graph = builder.build();

    auto root = Compiler::compile(graph);
    auto source = dynamic_cast<CollectionSourcePhysicalOperator*>(root.get());

    source->open();
    for (size_t i = 0; i < source->data().size(); ++i) {
        auto dummy = make_event(0);
        source->execute(dummy);
    }
    source->close();

    auto filter_ptr = root->next();
    auto sink_ptr = filter_ptr->next();
    auto sink = std::dynamic_pointer_cast<CollectSinkPhysicalOperator>(sink_ptr);
    ASSERT_EQ(sink->collected().size(), 2u);
    EXPECT_EQ(make_value_extractor()(sink->collected()[0]).raw(), 15);
    EXPECT_EQ(make_value_extractor()(sink->collected()[1]).raw(), 25);
}

}  // namespace
}  // namespace extream
