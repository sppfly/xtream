#include <gtest/gtest.h>

#include <memory>

#include "operators/physical/chain_context.h"
#include "operators/physical/collect_sink_physical_operator.h"
#include "operators/physical/collection_source_physical_operator.h"
#include "operators/physical/filter_physical_operator.h"
#include "operators/physical/map_physical_operator.h"

namespace extream {
namespace {

auto make_record(u64 value) -> Record {
    auto schema = std::make_shared<StreamSchema>();
    schema->add_field("value", TypeKind::U64);
    auto record = Record(schema);
    record.add_field(FieldValue::make_u64(value));
    return record;
}

auto make_event(u64 value, i64 ts = i64(0)) -> Event<Record> {
    return Event<Record>(make_record(value), ts);
}

auto make_value_extractor() {
    return [](const Event<Record>& event) -> u64 { return event.payload().field(0).as_u64(); };
}

TEST(MapPhysicalTest, TransformsEvent) {
    auto func = [](Event<Record>& e) -> Event<Record> {
        auto val = make_value_extractor()(e);
        return make_event(val + u64(1));
    };
    MapPhysicalOperator map(func);
    CollectSinkPhysicalOperator sink;
    ChainContext ctx;
    ctx.set_downstream(sink, ctx);

    auto event = make_event(u64(42));
    map.execute(ctx, event);

    ASSERT_EQ(sink.collected().size(), 1u);
    EXPECT_EQ(make_value_extractor()(sink.collected()[0]), u64(43));
}

TEST(MapPhysicalTest, MultipleEvents) {
    auto func = [](Event<Record>& e) -> Event<Record> {
        auto val = make_value_extractor()(e);
        return make_event(val * u64(2));
    };
    MapPhysicalOperator map(func);
    CollectSinkPhysicalOperator sink;
    ChainContext ctx;
    ctx.set_downstream(sink, ctx);

    for (u64 i : {u64(1), u64(2), u64(3)}) {
        auto event = make_event(i);
        map.execute(ctx, event);
    }

    ASSERT_EQ(sink.collected().size(), 3u);
    EXPECT_EQ(make_value_extractor()(sink.collected()[0]), u64(2));
    EXPECT_EQ(make_value_extractor()(sink.collected()[1]), u64(4));
    EXPECT_EQ(make_value_extractor()(sink.collected()[2]), u64(6));
}

TEST(FilterPhysicalTest, PassesMatchingEvents) {
    auto pred = [](Event<Record>& e) -> bool { return make_value_extractor()(e) > u64(10); };
    FilterPhysicalOperator filter(pred);
    CollectSinkPhysicalOperator sink;
    ChainContext ctx;
    ctx.set_downstream(sink, ctx);

    auto e1 = make_event(u64(5));
    auto e2 = make_event(u64(15));
    filter.execute(ctx, e1);
    filter.execute(ctx, e2);

    ASSERT_EQ(sink.collected().size(), 1u);
    EXPECT_EQ(make_value_extractor()(sink.collected()[0]), u64(15));
}

TEST(FilterPhysicalTest, DropsAllBelowThreshold) {
    auto pred = [](Event<Record>& e) -> bool { return make_value_extractor()(e) > u64(10); };
    FilterPhysicalOperator filter(pred);
    CollectSinkPhysicalOperator sink;
    ChainContext ctx;
    ctx.set_downstream(sink, ctx);

    for (u64 i : {u64(1), u64(3), u64(5), u64(7)}) {
        auto event = make_event(i);
        filter.execute(ctx, event);
    }

    EXPECT_TRUE(sink.collected().empty());
}

TEST(CollectionSourceTest, EmitsAllEvents) {
    auto data = std::vector{make_event(u64(10)), make_event(u64(20)), make_event(u64(30))};
    CollectionSourcePhysicalOperator source(data);
    CollectSinkPhysicalOperator sink;
    ChainContext ctx;
    ctx.set_downstream(sink, ctx);

    source.open(ctx);
    for (size_t i = 0; i < data.size(); ++i) {
        auto dummy = make_event(u64(0));
        source.execute(ctx, dummy);
    }
    source.close(ctx);

    ASSERT_EQ(sink.collected().size(), 3u);
    EXPECT_EQ(make_value_extractor()(sink.collected()[0]), u64(10));
    EXPECT_EQ(make_value_extractor()(sink.collected()[2]), u64(30));
}

TEST(ChainTest, SourceToSink) {
    auto data = std::vector{make_event(u64(5)), make_event(u64(10)), make_event(u64(15))};
    CollectionSourcePhysicalOperator source(data);
    CollectSinkPhysicalOperator sink;
    ChainContext ctx;
    ctx.set_downstream(sink, ctx);

    source.open(ctx);
    for (size_t i = 0; i < data.size(); ++i) {
        auto dummy = make_event(u64(0));
        source.execute(ctx, dummy);
    }
    source.close(ctx);

    ASSERT_EQ(sink.collected().size(), 3u);
}

TEST(ChainTest, SourceMapFilterSink) {
    auto data =
        std::vector{make_event(u64(1)), make_event(u64(2)), make_event(u64(3)), make_event(u64(4))};

    CollectionSourcePhysicalOperator source(data);
    MapPhysicalOperator map([](Event<Record>& e) {
        auto val = make_value_extractor()(e);
        return make_event(val * u64(10));
    });
    FilterPhysicalOperator filter(
        [](Event<Record>& e) { return make_value_extractor()(e) > u64(20); });
    CollectSinkPhysicalOperator sink;

    ChainContext sink_ctx;
    ChainContext filter_ctx;
    ChainContext map_ctx;
    ChainContext source_ctx;

    filter_ctx.set_downstream(sink, sink_ctx);
    map_ctx.set_downstream(filter, filter_ctx);
    source_ctx.set_downstream(map, map_ctx);

    source.open(source_ctx);
    for (size_t i = 0; i < data.size(); ++i) {
        auto dummy = make_event(u64(0));
        source.execute(source_ctx, dummy);
    }
    source.close(source_ctx);

    ASSERT_EQ(sink.collected().size(), 2u);
    EXPECT_EQ(make_value_extractor()(sink.collected()[0]), u64(30));
    EXPECT_EQ(make_value_extractor()(sink.collected()[1]), u64(40));
}

TEST(LifecycleTest, Ordering) {
    struct CallbackOperator : PhysicalOperator {
        std::vector<std::string> calls;

        void setup(OperatorContext&) override { calls.push_back("setup"); }
        void open(OperatorContext&) override { calls.push_back("open"); }
        void execute(OperatorContext&, Event<Record>&) override { calls.push_back("execute"); }
        void close(OperatorContext&) override { calls.push_back("close"); }
        void terminate(OperatorContext&) override { calls.push_back("terminate"); }
    };

    CallbackOperator op;
    ChainContext ctx;
    auto event = make_event(u64(0));
    op.setup(ctx);
    op.open(ctx);
    op.execute(ctx, event);
    op.execute(ctx, event);
    op.close(ctx);
    op.terminate(ctx);

    std::vector<std::string> expected = {"setup",   "open",  "execute",
                                         "execute", "close", "terminate"};
    EXPECT_EQ(op.calls, expected);
}

}  // namespace
}  // namespace extream
