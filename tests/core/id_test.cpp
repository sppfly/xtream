#include "core/id.h"

#include <gtest/gtest.h>

#include <set>
#include <type_traits>
#include <unordered_set>

namespace xtream {
namespace {

struct OperatorTag {};
struct StreamTag {};

using OperatorId = Id<OperatorTag>;
using StreamId = Id<StreamTag>;

TEST(IdTest, ConstructFromU64) {
    OperatorId id(u64(42));
    EXPECT_EQ(id.raw().raw(), 42u);
}

TEST(IdTest, NoImplicitFromRaw) {
    static_assert(!std::is_convertible_v<u64, OperatorId>);
    static_assert(!std::is_convertible_v<OperatorId, u64>);
}

TEST(IdTest, Equality) {
    OperatorId a(u64(1));
    OperatorId b(u64(1));
    OperatorId c(u64(2));

    EXPECT_EQ(a, b);
    EXPECT_NE(a, c);
}

TEST(IdTest, Ordering) {
    OperatorId a(u64(1));
    OperatorId b(u64(2));

    EXPECT_LT(a, b);
    EXPECT_LE(a, a);
    EXPECT_GT(b, a);
    EXPECT_GE(b, b);
}

TEST(IdTest, DifferentTagsIncompatible) {
    OperatorId op_id(u64(1));
    StreamId stream_id(u64(1));
    static_assert(!std::is_same_v<OperatorId, StreamId>);
}

TEST(IdTest, CopyAndAssign) {
    OperatorId a(u64(10));
    OperatorId b = a;
    OperatorId c(u64(0));
    c = a;

    EXPECT_EQ(a, b);
    EXPECT_EQ(a, c);
}

TEST(IdTest, UsedInSet) {
    std::set<OperatorId> ids;
    ids.insert(OperatorId(u64(3)));
    ids.insert(OperatorId(u64(1)));
    ids.insert(OperatorId(u64(2)));

    EXPECT_EQ(ids.size(), 3u);
    EXPECT_EQ(ids.begin()->raw().raw(), 1u);
}

TEST(IdTest, UsedInUnorderedSet) {
    std::unordered_set<OperatorId> ids;
    ids.insert(OperatorId(u64(10)));
    ids.insert(OperatorId(u64(20)));

    EXPECT_EQ(ids.size(), 2u);
    EXPECT_NE(ids.find(OperatorId(u64(10))), ids.end());
    EXPECT_EQ(ids.find(OperatorId(u64(99))), ids.end());
}

}  // namespace
}  // namespace xtream
