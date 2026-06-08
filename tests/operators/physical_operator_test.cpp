#include <gtest/gtest.h>

#include <memory>

#include "operators/physical/filter_physical_operator.h"
#include "operators/physical/map_physical_operator.h"
#include "operators/physical/sink_physical_operator.h"
#include "operators/physical/source_physical_operator.h"

namespace xtream {
namespace {

auto make_record(u64 value) -> Record {
    auto schema = std::make_shared<StreamSchema>();
    schema->add_field("value", TypeKind::U64);
    auto record = Record(schema);
    record.add_field(FieldValue::make_u64(value));
    return record;
}

auto make_event(u64 value, u64 ts = u64(0)) -> Event<Record> {
    return Event<Record>(make_record(value), ts);
}

auto make_value_extractor() {
    return [](const Event<Record>& event) -> u64 { return event.payload().field(0).as_u64(); };
}

TEST(MapPhysicalTest, TransformsEvent) {
    auto map = std::make_shared<MapPhysicalOperator>([](Event<Record>& e) -> Event<Record> {
        auto val = make_value_extractor()(e);
        return make_event(val + u64(1));
    });
    std::vector<Event<Record>> collected;
    auto sink = std::make_shared<SinkPhysicalOperator>(
        [&collected](Event<Record>& e) { collected.push_back(e); });
    map->set_next(sink);

    auto event = make_event(u64(42));
    map->execute(event);

    ASSERT_EQ(collected.size(), 1u);
    EXPECT_EQ(make_value_extractor()(collected[0]), u64(43));
}

TEST(MapPhysicalTest, MultipleEvents) {
    auto map = std::make_shared<MapPhysicalOperator>([](Event<Record>& e) -> Event<Record> {
        auto val = make_value_extractor()(e);
        return make_event(val * u64(2));
    });
    std::vector<Event<Record>> collected;
    auto sink = std::make_shared<SinkPhysicalOperator>(
        [&collected](Event<Record>& e) { collected.push_back(e); });
    map->set_next(sink);

    for (u64 i : {u64(1), u64(2), u64(3)}) {
        auto event = make_event(i);
        map->execute(event);
    }

    ASSERT_EQ(collected.size(), 3u);
    EXPECT_EQ(make_value_extractor()(collected[0]), u64(2));
    EXPECT_EQ(make_value_extractor()(collected[1]), u64(4));
    EXPECT_EQ(make_value_extractor()(collected[2]), u64(6));
}

TEST(FilterPhysicalTest, PassesMatchingEvents) {
    auto filter = std::make_shared<FilterPhysicalOperator>(
        [](Event<Record>& e) -> bool { return make_value_extractor()(e) > u64(10); });
    std::vector<Event<Record>> collected;
    auto sink = std::make_shared<SinkPhysicalOperator>(
        [&collected](Event<Record>& e) { collected.push_back(e); });
    filter->set_next(sink);

    auto e1 = make_event(u64(5));
    auto e2 = make_event(u64(15));
    filter->execute(e1);
    filter->execute(e2);

    ASSERT_EQ(collected.size(), 1u);
    EXPECT_EQ(make_value_extractor()(collected[0]), u64(15));
}

TEST(FilterPhysicalTest, DropsAllBelowThreshold) {
    auto filter = std::make_shared<FilterPhysicalOperator>(
        [](Event<Record>& e) -> bool { return make_value_extractor()(e) > u64(10); });
    std::vector<Event<Record>> collected;
    auto sink = std::make_shared<SinkPhysicalOperator>(
        [&collected](Event<Record>& e) { collected.push_back(e); });
    filter->set_next(sink);

    for (u64 i : {u64(1), u64(3), u64(5), u64(7)}) {
        auto event = make_event(i);
        filter->execute(event);
    }

    EXPECT_TRUE(collected.empty());
}

TEST(SourcePhysicalTest, EmitsEventsFromFunction) {
    std::vector<Event<Record>> collected;
    auto sink = std::make_shared<SinkPhysicalOperator>(
        [&collected](Event<Record>& e) { collected.push_back(e); });

    std::vector<Event<Record>> data = {make_event(u64(10)), make_event(u64(20)),
                                       make_event(u64(30))};
    size_t idx = 0;
    auto source = std::make_shared<SourcePhysicalOperator>(
        [&data, &idx]() -> Event<Record> { return data[idx++]; });
    source->set_next(sink);

    source->open();
    for (size_t i = 0; i < data.size(); ++i) {
        auto dummy = make_event(u64(0));
        source->execute(dummy);
    }
    source->close();

    ASSERT_EQ(collected.size(), 3u);
    EXPECT_EQ(make_value_extractor()(collected[0]), u64(10));
    EXPECT_EQ(make_value_extractor()(collected[2]), u64(30));
}

TEST(ChainTest, SourceToSink) {
    std::vector<Event<Record>> collected;
    auto sink = std::make_shared<SinkPhysicalOperator>(
        [&collected](Event<Record>& e) { collected.push_back(e); });

    std::vector<Event<Record>> data = {make_event(u64(5)), make_event(u64(10)),
                                       make_event(u64(15))};
    size_t idx = 0;
    auto source = std::make_shared<SourcePhysicalOperator>(
        [&data, &idx]() -> Event<Record> { return data[idx++]; });
    source->set_next(sink);

    source->open();
    for (size_t i = 0; i < data.size(); ++i) {
        auto dummy = make_event(u64(0));
        source->execute(dummy);
    }
    source->close();

    ASSERT_EQ(collected.size(), 3u);
}

TEST(ChainTest, SourceMapFilterSink) {
    std::vector<Event<Record>> collected;
    auto sink = std::make_shared<SinkPhysicalOperator>(
        [&collected](Event<Record>& e) { collected.push_back(e); });

    auto filter = std::make_shared<FilterPhysicalOperator>(
        [](Event<Record>& e) { return make_value_extractor()(e) > u64(20); });
    auto map = std::make_shared<MapPhysicalOperator>([](Event<Record>& e) {
        auto val = make_value_extractor()(e);
        return make_event(val * u64(10));
    });

    std::vector<Event<Record>> data = {make_event(u64(1)), make_event(u64(2)), make_event(u64(3)),
                                       make_event(u64(4))};
    size_t idx = 0;
    auto source = std::make_shared<SourcePhysicalOperator>(
        [&data, &idx]() -> Event<Record> { return data[idx++]; });

    source->set_next(map);
    map->set_next(filter);
    filter->set_next(sink);

    source->open();
    for (size_t i = 0; i < data.size(); ++i) {
        auto dummy = make_event(u64(0));
        source->execute(dummy);
    }
    source->close();

    ASSERT_EQ(collected.size(), 2u);
    EXPECT_EQ(make_value_extractor()(collected[0]), u64(30));
    EXPECT_EQ(make_value_extractor()(collected[1]), u64(40));
}

TEST(LifecycleTest, Ordering) {
    struct CallbackOperator : PhysicalOperator {
        std::vector<std::string> calls;

        void setup() override { calls.push_back("setup"); }
        void open() override { calls.push_back("open"); }
        void execute(Event<Record>&) override { calls.push_back("execute"); }
        void close() override { calls.push_back("close"); }
        void terminate() override { calls.push_back("terminate"); }
    };

    auto op = std::make_shared<CallbackOperator>();
    op->setup();
    op->open();
    auto event = make_event(u64(0));
    op->execute(event);
    op->execute(event);
    op->close();
    op->terminate();

    std::vector<std::string> expected = {"setup",   "open",  "execute",
                                         "execute", "close", "terminate"};
    EXPECT_EQ(op->calls, expected);
}

}  // namespace
}  // namespace xtream
