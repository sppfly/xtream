#include "engine/pipeline.h"

#include <gtest/gtest.h>

#include "graph/dataflow_graph_builder.h"
#include "operators/logical/filter_logical_operator.h"
#include "operators/logical/map_logical_operator.h"
#include "operators/logical/sink_logical_operator.h"
#include "operators/logical/source_logical_operator.h"

namespace extream {
namespace {

auto make_record(int v) -> Record {
    auto schema = std::make_shared<StreamSchema>();
    schema->add_field("value", TypeKind::I32);
    auto record = Record(schema);
    record.add_field(FieldValue::make_i32(i32(v)));
    return record;
}

auto make_value_extractor() {
    return [](const Event<Record>& event) -> i32 { return event.payload().field(0).as_i32(); };
}

TEST(PipelineTest, SourceToSink) {
    std::vector<Event<Record>> collected;
    int counter = 0;

    auto graph = DataflowGraphBuilder()
                     .source([&counter]() -> Event<Record> {
                         ++counter;
                         return Event<Record>(make_record(counter), i64(counter));
                     })
                     .sink([&collected](Event<Record>& e) { collected.push_back(e); })
                     .build();

    Pipeline pipeline(std::move(graph));
    pipeline.run(3);

    ASSERT_EQ(collected.size(), 3u);
    EXPECT_EQ(make_value_extractor()(collected[0]).raw(), 1);
    EXPECT_EQ(make_value_extractor()(collected[1]).raw(), 2);
    EXPECT_EQ(make_value_extractor()(collected[2]).raw(), 3);
}

TEST(PipelineTest, SourceMapFilterSink) {
    std::vector<Event<Record>> collected;
    int counter = 0;

    auto graph =
        DataflowGraphBuilder()
            .source([&counter]() -> Event<Record> {
                ++counter;
                return Event<Record>(make_record(counter), i64(0));
            })
            .map([](Event<Record>& e) -> Event<Record> {
                auto val = make_value_extractor()(e);
                return Event<Record>(make_record(val.raw() * 10), e.timestamp());
            })
            .filter([](Event<Record>& e) -> bool { return make_value_extractor()(e).raw() > 20; })
            .sink([&collected](Event<Record>& e) { collected.push_back(e); })
            .build();

    Pipeline pipeline(std::move(graph));
    pipeline.run(4);

    ASSERT_EQ(collected.size(), 2u);
    EXPECT_EQ(make_value_extractor()(collected[0]).raw(), 30);
    EXPECT_EQ(make_value_extractor()(collected[1]).raw(), 40);
}

}  // namespace
}  // namespace extream
