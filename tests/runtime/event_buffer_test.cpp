#include <gtest/gtest.h>

#include "core/event.h"
#include "core/record.h"
#include "runtime/event_buffer.h"

namespace xtream {
namespace {

auto make_event(u64 ts) -> Event<Record> {
    return Event<Record>(Record(nullptr), ts);
}

TEST(EventBufferTest, InsertInOrder) {
    EventBuffer buffer;
    buffer.insert(make_event(u64(100)));
    buffer.insert(make_event(u64(200)));
    buffer.insert(make_event(u64(300)));
    EXPECT_EQ(buffer.size(), u64(3));
}

TEST(EventBufferTest, InsertOutOfOrder) {
    EventBuffer buffer;
    buffer.insert(make_event(u64(300)));
    buffer.insert(make_event(u64(100)));
    buffer.insert(make_event(u64(200)));
    EXPECT_EQ(buffer.size(), u64(3));
}

TEST(EventBufferTest, ExtractRange) {
    EventBuffer buffer;
    buffer.insert(make_event(u64(100)));
    buffer.insert(make_event(u64(200)));
    buffer.insert(make_event(u64(300)));
    buffer.insert(make_event(u64(400)));
    buffer.insert(make_event(u64(500)));

    auto extracted = buffer.extract(u64(200), u64(400));
    ASSERT_EQ(extracted.size(), 2u);
    EXPECT_EQ(extracted[0].timestamp(), u64(200));
    EXPECT_EQ(extracted[1].timestamp(), u64(300));
    EXPECT_EQ(buffer.size(), u64(3));
}

TEST(EventBufferTest, ExtractEmptyRange) {
    EventBuffer buffer;
    buffer.insert(make_event(u64(100)));
    buffer.insert(make_event(u64(200)));

    auto extracted = buffer.extract(u64(300), u64(400));
    EXPECT_TRUE(extracted.empty());
    EXPECT_EQ(buffer.size(), u64(2));
}

TEST(EventBufferTest, ExtractAll) {
    EventBuffer buffer;
    buffer.insert(make_event(u64(100)));
    buffer.insert(make_event(u64(200)));

    auto extracted = buffer.extract(u64(0), u64(300));
    ASSERT_EQ(extracted.size(), 2u);
    EXPECT_TRUE(buffer.empty());
}

TEST(EventBufferTest, CleanupBefore) {
    EventBuffer buffer;
    buffer.insert(make_event(u64(100)));
    buffer.insert(make_event(u64(200)));
    buffer.insert(make_event(u64(300)));
    buffer.insert(make_event(u64(400)));

    buffer.cleanup_before(u64(250));
    EXPECT_EQ(buffer.size(), u64(2));
}

TEST(EventBufferTest, CleanupBeforeAll) {
    EventBuffer buffer;
    buffer.insert(make_event(u64(100)));
    buffer.insert(make_event(u64(200)));

    buffer.cleanup_before(u64(500));
    EXPECT_TRUE(buffer.empty());
}

TEST(EventBufferTest, CleanupBeforeNone) {
    EventBuffer buffer;
    buffer.insert(make_event(u64(100)));
    buffer.insert(make_event(u64(200)));

    buffer.cleanup_before(u64(0));
    EXPECT_EQ(buffer.size(), u64(2));
}

TEST(EventBufferTest, Empty) {
    EventBuffer buffer;
    EXPECT_TRUE(buffer.empty());
    EXPECT_EQ(buffer.size(), u64(0));

    auto extracted = buffer.extract(u64(0), u64(100));
    EXPECT_TRUE(extracted.empty());

    buffer.cleanup_before(u64(100));
    EXPECT_TRUE(buffer.empty());
}

}  // namespace
}  // namespace xtream
