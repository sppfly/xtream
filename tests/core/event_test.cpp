#include "core/event.h"

#include <gtest/gtest.h>

#include <string>

namespace extream {
namespace {

TEST(EventTest, ConstructWithPayloadAndTimestamp) {
    Event<int> event(42, i64(1000));
    EXPECT_EQ(event.payload(), 42);
    EXPECT_EQ(event.timestamp().raw(), 1000);
}

TEST(EventTest, PayloadIsMutableRef) {
    Event<std::string> event("hello", i64(0));
    EXPECT_EQ(event.payload(), "hello");
}

TEST(EventTest, CopyConstructor) {
    Event<int> original(10, i64(500));
    Event<int> copy = original;
    EXPECT_EQ(copy.payload(), 10);
    EXPECT_EQ(copy.timestamp().raw(), 500);
}

TEST(EventTest, MoveConstructor) {
    Event<int> original(10, i64(500));
    Event<int> moved = std::move(original);
    EXPECT_EQ(moved.payload(), 10);
    EXPECT_EQ(moved.timestamp().raw(), 500);
}

TEST(EventTest, SetAndGetMetadata) {
    Event<int> event(1, i64(0));
    event.set_metadata("key1", "value1");
    event.set_metadata("key2", "value2");

    auto v1 = event.metadata("key1");
    ASSERT_TRUE(v1.has_value());
    EXPECT_EQ(*v1, "value1");

    auto v2 = event.metadata("key2");
    ASSERT_TRUE(v2.has_value());
    EXPECT_EQ(*v2, "value2");
}

TEST(EventTest, MetadataNotFoundReturnsNullopt) {
    Event<int> event(1, i64(0));
    auto result = event.metadata("nonexistent");
    EXPECT_FALSE(result.has_value());
}

TEST(EventTest, MetadataOverwrite) {
    Event<int> event(1, i64(0));
    event.set_metadata("key", "old");
    event.set_metadata("key", "new");
    EXPECT_EQ(*event.metadata("key"), "new");
}

TEST(EventTest, EventsAreOrderedByTimestamp) {
    Event<int> e1(1, i64(100));
    Event<int> e2(2, i64(200));
    EXPECT_LT(e1.timestamp().raw(), e2.timestamp().raw());
}

TEST(EventTest, DifferentPayloadTypes) {
    Event<double> e1(3.14, i64(0));
    Event<std::string> e2("data", i64(0));
    EXPECT_DOUBLE_EQ(e1.payload(), 3.14);
    EXPECT_EQ(e2.payload(), "data");
}

}  // namespace
}  // namespace extream
