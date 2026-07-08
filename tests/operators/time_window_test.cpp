#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "core/event.h"
#include "core/record.h"
#include "core/types/window_spec.h"
#include "core/watermark.h"
#include "operators/physical/sink_physical_operator.h"
#include "operators/physical/time_window_physical_operator.h"
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

auto sum_values(const std::vector<Event<Record>>& events) -> Event<Record> {
    u64 sum{0};
    for (const auto& e : events) {
        sum = sum + e.payload().field(0).as_u64();
    }
    return make_event(sum, u64(0));
}

auto extract_value(const Event<Record>& e) -> u64 {
    return e.payload().field(0).as_u64();
}

struct CapturingSink : PhysicalOperator {
    std::vector<Event<Record>> events;

    std::string_view type_name() const override { return "CapturingSink"; }
    void setup() override {}
    void open() override {}
    void execute(StreamElement& elem) override {
        if (auto* e = std::get_if<Event<Record>>(&elem)) {
            events.push_back(*e);
        }
    }
    void close() override {}
    void terminate() override {}
};

TEST(TimeWindowTest, TumblingWindowFiresOnWatermark) {
    auto sink = std::make_shared<CapturingSink>();
    // tumbling: size=5, step=5
    auto win =
        std::make_shared<TimeWindowPhysicalOperator>(time_tumbling_window(usize(5)), sum_values);
    win->set_next(sink);

    win->open();
    // events in [0,5) and [5,10)
    StreamElement e1{make_event(u64(1), u64(0))};
    StreamElement e2{make_event(u64(2), u64(3))};
    StreamElement e3{make_event(u64(3), u64(7))};
    StreamElement e4{make_event(u64(4), u64(9))};
    win->execute(e1);
    win->execute(e2);
    win->execute(e3);
    win->execute(e4);
    // no output yet — watermark hasn't arrived
    EXPECT_TRUE(sink->events.empty());

    // watermark = 5 fires window [0,5)
    StreamElement wm1{Watermark(u64(5), u64(0))};
    win->execute(wm1);
    ASSERT_EQ(sink->events.size(), 1u);
    EXPECT_EQ(extract_value(sink->events[0]), u64(3));  // 1+2

    // watermark = 10 fires window [5,10)
    StreamElement wm2{Watermark(u64(10), u64(0))};
    win->execute(wm2);
    ASSERT_EQ(sink->events.size(), 2u);
    EXPECT_EQ(extract_value(sink->events[1]), u64(7));  // 3+4

    win->close();
}

TEST(TimeWindowTest, SlidingWindowFiresMultipleOnWatermark) {
    auto sink = std::make_shared<CapturingSink>();
    // sliding: size=5, step=1
    auto win =
        std::make_shared<TimeWindowPhysicalOperator>(time_window(usize(5), usize(1)), sum_values);
    win->set_next(sink);

    win->open();
    // events at ts=0,1,2,3,4,5
    for (u64 i : {u64(0), u64(1), u64(2), u64(3), u64(4), u64(5)}) {
        StreamElement e{make_event(u64(1), i)};
        win->execute(e);
    }

    // next_window_end_ starts at 1 (step=1)
    // watermark=5 fires windows with end<=5: [0,5) only has events at 0,1,2,3,4
    // windows end=1,2,3,4 are empty (start underflows to 0, extract(0,1) etc.)
    //   [0,1): event at 0 → 1 event
    //   [0,2): events at 0,1 → 2 events
    //   [0,3): events at 0,1,2 → 3 events
    //   [0,4): events at 0,1,2,3 → 4 events
    //   [0,5): events at 0,1,2,3,4 → 5 events
    StreamElement wm{Watermark(u64(5), u64(0))};
    win->execute(wm);
    ASSERT_EQ(sink->events.size(), 5u);
    EXPECT_EQ(extract_value(sink->events[0]), u64(1));  // [0,1)
    EXPECT_EQ(extract_value(sink->events[1]), u64(2));  // [0,2)
    EXPECT_EQ(extract_value(sink->events[2]), u64(3));  // [0,3)
    EXPECT_EQ(extract_value(sink->events[3]), u64(4));  // [0,4)
    EXPECT_EQ(extract_value(sink->events[4]), u64(5));  // [0,5)

    // watermark=6 fires [1,6): events at 1,2,3,4,5 → 5
    StreamElement wm2{Watermark(u64(6), u64(0))};
    win->execute(wm2);
    ASSERT_EQ(sink->events.size(), 6u);
    EXPECT_EQ(extract_value(sink->events[5]), u64(5));

    win->close();
}

TEST(TimeWindowTest, OutOfOrderEventsHandledCorrectly) {
    auto sink = std::make_shared<CapturingSink>();
    auto win =
        std::make_shared<TimeWindowPhysicalOperator>(time_tumbling_window(usize(10)), sum_values);
    win->set_next(sink);

    win->open();
    // out-of-order: 15, 5, 10
    StreamElement e1{make_event(u64(1), u64(15))};
    StreamElement e2{make_event(u64(2), u64(5))};
    StreamElement e3{make_event(u64(3), u64(10))};
    win->execute(e1);
    win->execute(e2);
    win->execute(e3);

    // watermark=10 fires [0,10): events at 5 = 2
    StreamElement wm{Watermark(u64(10), u64(0))};
    win->execute(wm);
    ASSERT_EQ(sink->events.size(), 1u);
    EXPECT_EQ(extract_value(sink->events[0]), u64(2));

    // watermark=20 fires [10,20): events at 15, 10 = 1+3 = 4
    StreamElement wm2{Watermark(u64(20), u64(0))};
    win->execute(wm2);
    ASSERT_EQ(sink->events.size(), 2u);
    EXPECT_EQ(extract_value(sink->events[1]), u64(4));

    win->close();
}

TEST(TimeWindowTest, NoFireBeforeWatermark) {
    auto sink = std::make_shared<CapturingSink>();
    auto win =
        std::make_shared<TimeWindowPhysicalOperator>(time_tumbling_window(usize(5)), sum_values);
    win->set_next(sink);

    win->open();
    StreamElement e1{make_event(u64(1), u64(0))};
    StreamElement e2{make_event(u64(2), u64(4))};
    win->execute(e1);
    win->execute(e2);
    // no watermark → no output
    EXPECT_TRUE(sink->events.empty());
    win->close();
}

TEST(TimeWindowTest, CloseFlushesRemainingWindows) {
    auto sink = std::make_shared<CapturingSink>();
    auto win =
        std::make_shared<TimeWindowPhysicalOperator>(time_tumbling_window(usize(5)), sum_values);
    win->set_next(sink);

    win->open();
    StreamElement e1{make_event(u64(1), u64(0))};
    StreamElement e2{make_event(u64(2), u64(3))};
    win->execute(e1);
    win->execute(e2);
    EXPECT_TRUE(sink->events.empty());
    // close flushes all remaining windows
    win->close();
    ASSERT_EQ(sink->events.size(), 1u);
    EXPECT_EQ(extract_value(sink->events[0]), u64(3));  // 1+2 in [0,5)
}

TEST(TimeWindowTest, EmptyWindowProducesEmptyResult) {
    auto sink = std::make_shared<CapturingSink>();
    auto win =
        std::make_shared<TimeWindowPhysicalOperator>(time_tumbling_window(usize(5)), sum_values);
    win->set_next(sink);

    win->open();
    // no events, just watermark
    StreamElement wm{Watermark(u64(10), u64(0))};
    win->execute(wm);
    // first_window_end never initialized (no events), so on_watermark does nothing
    EXPECT_TRUE(sink->events.empty());
    win->close();
}

TEST(TimeWindowTest, WatermarkDoesNotFireFutureWindows) {
    auto sink = std::make_shared<CapturingSink>();
    auto win =
        std::make_shared<TimeWindowPhysicalOperator>(time_tumbling_window(usize(5)), sum_values);
    win->set_next(sink);

    win->open();
    StreamElement e1{make_event(u64(1), u64(0))};
    win->execute(e1);

    // watermark=3 < next_window_end=5, no fire
    StreamElement wm{Watermark(u64(3), u64(0))};
    win->execute(wm);
    EXPECT_TRUE(sink->events.empty());

    win->close();
}

TEST(TimeWindowTest, FirstEventAtNonZeroTimestamp) {
    // size=5, step=1, first event at ts=3
    // next_window_end_ starts at 1 (step=1)
    // watermark=5 fires windows end=1..5
    //   [0,1), [0,2), [0,3) are empty (no events)
    //   [0,4): event at 3 → 1
    //   [0,5): event at 3 → 1
    auto sink = std::make_shared<CapturingSink>();
    auto win =
        std::make_shared<TimeWindowPhysicalOperator>(time_window(usize(5), usize(1)), sum_values);
    win->set_next(sink);

    win->open();
    StreamElement e{make_event(u64(1), u64(3))};
    win->execute(e);

    StreamElement wm{Watermark(u64(5), u64(0))};
    win->execute(wm);
    // only [0,4) and [0,5) have the event at ts=3
    ASSERT_EQ(sink->events.size(), 2u);
    EXPECT_EQ(extract_value(sink->events[0]), u64(1));  // [0,4)
    EXPECT_EQ(extract_value(sink->events[1]), u64(1));  // [0,5)
    win->close();
}

}  // namespace
}  // namespace xtream
