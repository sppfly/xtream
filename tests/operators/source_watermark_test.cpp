#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "core/event.h"
#include "core/record.h"
#include "core/watermark.h"
#include "operators/physical/sink_physical_operator.h"
#include "operators/physical/source_physical_operator.h"
#include "runtime/stream_element.h"

namespace xtream {
namespace {

auto make_record(u64 value) -> Record {
    auto schema = std::make_shared<StreamSchema>();
    schema->add_field("value", TypeKind::U64);
    auto record = Record(schema);
    record.add_field(FieldValue::make_u64(value));
    return record;
}

auto make_event(u64 value, u64 ts) -> Event<Record> {
    return Event<Record>(make_record(value), ts);
}

// 捕获下游所有 StreamElement（Event 和 Watermark 都收）
struct CapturingSink : PhysicalOperator {
    std::vector<Event<Record>> events;
    std::vector<Watermark> watermarks;

    std::string_view type_name() const override { return "CapturingSink"; }
    void setup() override {}
    void open() override {}
    void execute(StreamElement& elem) override {
        if (auto* e = std::get_if<Event<Record>>(&elem)) {
            events.push_back(*e);
        } else if (auto* w = std::get_if<Watermark>(&elem)) {
            watermarks.push_back(*w);
        }
    }
    void close() override {}
    void terminate() override {}
};

TEST(SourceWatermarkTest, DefaultIsStrictOrdering) {
    auto sink = std::make_shared<CapturingSink>();

    std::vector<Event<Record>> data = {make_event(u64(1), u64(100)), make_event(u64(2), u64(200)),
                                       make_event(u64(3), u64(300))};
    size_t idx = 0;
    // 默认 allowed_lateness=0, interval=100 → 事件期间不发，close flush 一个 watermark=300
    auto source = std::make_shared<SourcePhysicalOperator>(
        [&data, &idx]() -> std::optional<Event<Record>> { return data[idx++]; });
    source->set_next(sink);

    source->open();
    for (size_t i = 0; i < data.size(); ++i) {
        StreamElement dummy{make_event(u64(0), u64(0))};
        source->execute(dummy);
    }
    source->close();

    ASSERT_EQ(sink->events.size(), 3u);
    // 默认 lateness=0，close 时 flush watermark = max_ts = 300
    ASSERT_EQ(sink->watermarks.size(), 1u);
    EXPECT_EQ(sink->watermarks[0].timestamp(), u64(300));
}

TEST(SourceWatermarkTest, EmitsWatermarkEqualToMaxTsWhenLatenessZero) {
    auto sink = std::make_shared<CapturingSink>();

    std::vector<Event<Record>> data = {make_event(u64(1), u64(100)), make_event(u64(2), u64(200)),
                                       make_event(u64(3), u64(300))};
    size_t idx = 0;
    // allowed_lateness=0, interval=1 → 每条事件都发 watermark = max_ts
    auto source = std::make_shared<SourcePhysicalOperator>(
        [&data, &idx]() -> std::optional<Event<Record>> { return data[idx++]; }, u64(0), usize(1));
    source->set_next(sink);

    source->open();
    for (size_t i = 0; i < data.size(); ++i) {
        StreamElement dummy{make_event(u64(0), u64(0))};
        source->execute(dummy);
    }
    source->close();

    ASSERT_EQ(sink->events.size(), 3u);
    ASSERT_EQ(sink->watermarks.size(), 3u);
    EXPECT_EQ(sink->watermarks[0].timestamp(), u64(100));
    EXPECT_EQ(sink->watermarks[1].timestamp(), u64(200));
    EXPECT_EQ(sink->watermarks[2].timestamp(), u64(300));
}

TEST(SourceWatermarkTest, EmitsWatermarkWithLateness) {
    auto sink = std::make_shared<CapturingSink>();

    std::vector<Event<Record>> data = {make_event(u64(1), u64(100)), make_event(u64(2), u64(200)),
                                       make_event(u64(3), u64(300))};
    size_t idx = 0;
    // allowed_lateness=50, interval=1 → watermark = max_ts - 50
    auto source = std::make_shared<SourcePhysicalOperator>(
        [&data, &idx]() -> std::optional<Event<Record>> { return data[idx++]; }, u64(50), usize(1));
    source->set_next(sink);

    source->open();
    for (size_t i = 0; i < data.size(); ++i) {
        StreamElement dummy{make_event(u64(0), u64(0))};
        source->execute(dummy);
    }
    source->close();

    ASSERT_EQ(sink->watermarks.size(), 3u);
    EXPECT_EQ(sink->watermarks[0].timestamp(), u64(50));
    EXPECT_EQ(sink->watermarks[1].timestamp(), u64(150));
    EXPECT_EQ(sink->watermarks[2].timestamp(), u64(250));
}

TEST(SourceWatermarkTest, WatermarkDoesNotGoBackwards) {
    auto sink = std::make_shared<CapturingSink>();

    // 乱序：300, 100, 200
    std::vector<Event<Record>> data = {make_event(u64(1), u64(300)), make_event(u64(2), u64(100)),
                                       make_event(u64(3), u64(200))};
    size_t idx = 0;
    auto source = std::make_shared<SourcePhysicalOperator>(
        [&data, &idx]() -> std::optional<Event<Record>> { return data[idx++]; }, u64(0), usize(1));
    source->set_next(sink);

    source->open();
    for (size_t i = 0; i < data.size(); ++i) {
        StreamElement dummy{make_event(u64(0), u64(0))};
        source->execute(dummy);
    }
    source->close();

    // 第一条发 300，第二三条 wm_ts<300 不发
    ASSERT_EQ(sink->watermarks.size(), 1u);
    EXPECT_EQ(sink->watermarks[0].timestamp(), u64(300));
}

TEST(SourceWatermarkTest, PeriodicEmissionInterval) {
    auto sink = std::make_shared<CapturingSink>();

    // 5 条事件，interval=2 → 在第 2、4 条后发 watermark，close 再 flush 一个
    std::vector<Event<Record>> data = {make_event(u64(1), u64(100)), make_event(u64(2), u64(200)),
                                       make_event(u64(3), u64(300)), make_event(u64(4), u64(400)),
                                       make_event(u64(5), u64(500))};
    size_t idx = 0;
    auto source = std::make_shared<SourcePhysicalOperator>(
        [&data, &idx]() -> std::optional<Event<Record>> { return data[idx++]; }, u64(0), usize(2));
    source->set_next(sink);

    source->open();
    for (size_t i = 0; i < data.size(); ++i) {
        StreamElement dummy{make_event(u64(0), u64(0))};
        source->execute(dummy);
    }
    source->close();

    // 第 2 条后发 200，第 4 条后发 400，close flush 发 500
    ASSERT_EQ(sink->watermarks.size(), 3u);
    EXPECT_EQ(sink->watermarks[0].timestamp(), u64(200));
    EXPECT_EQ(sink->watermarks[1].timestamp(), u64(400));
    EXPECT_EQ(sink->watermarks[2].timestamp(), u64(500));
}

TEST(SourceWatermarkTest, CloseFlushesRemainingWatermark) {
    auto sink = std::make_shared<CapturingSink>();

    // 3 条事件，interval=100 → 中间不发，close 时 flush
    std::vector<Event<Record>> data = {make_event(u64(1), u64(100)), make_event(u64(2), u64(200)),
                                       make_event(u64(3), u64(300))};
    size_t idx = 0;
    auto source = std::make_shared<SourcePhysicalOperator>(
        [&data, &idx]() -> std::optional<Event<Record>> { return data[idx++]; }, u64(0),
        usize(100));
    source->set_next(sink);

    source->open();
    for (size_t i = 0; i < data.size(); ++i) {
        StreamElement dummy{make_event(u64(0), u64(0))};
        source->execute(dummy);
    }
    source->close();

    ASSERT_EQ(sink->watermarks.size(), 1u);
    EXPECT_EQ(sink->watermarks[0].timestamp(), u64(300));
}

TEST(SourceWatermarkTest, EmptyStreamEmitsNoWatermark) {
    auto sink = std::make_shared<CapturingSink>();

    auto source = std::make_shared<SourcePhysicalOperator>(
        []() -> std::optional<Event<Record>> { return std::nullopt; }, u64(0), usize(1));
    source->set_next(sink);

    source->open();
    StreamElement dummy{make_event(u64(0), u64(0))};
    source->execute(dummy);
    source->close();

    // 第一条 execute 就返回 nullopt → done_=true，没事件进来
    // close 时 events_since_watermark_=0，不 flush
    EXPECT_TRUE(sink->events.empty());
    EXPECT_TRUE(sink->watermarks.empty());
}

}  // namespace
}  // namespace xtream
