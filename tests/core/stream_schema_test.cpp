#include "core/stream_schema.h"

#include <gtest/gtest.h>

namespace xtream {
namespace {

TEST(StreamSchemaTest, EmptySchema) {
    StreamSchema schema;
    EXPECT_EQ(schema.field_count(), 0u);
    EXPECT_TRUE(schema.fields().empty());
}

TEST(StreamSchemaTest, AddAndGetFields) {
    StreamSchema schema;
    schema.add_field("id", TypeKind::I64);
    schema.add_field("name", TypeKind::String);
    schema.add_field("score", TypeKind::F64);

    EXPECT_EQ(schema.field_count(), 3u);
    EXPECT_EQ(schema.fields()[0].name, "id");
    EXPECT_EQ(schema.fields()[0].type, TypeKind::I64);
    EXPECT_EQ(schema.fields()[1].name, "name");
    EXPECT_EQ(schema.fields()[1].type, TypeKind::String);
    EXPECT_EQ(schema.fields()[2].name, "score");
    EXPECT_EQ(schema.fields()[2].type, TypeKind::F64);
}

TEST(StreamSchemaTest, FieldIndexByName) {
    StreamSchema schema;
    schema.add_field("x", TypeKind::I32);
    schema.add_field("y", TypeKind::F32);

    auto idx_x = schema.field_index("x");
    ASSERT_TRUE(idx_x.has_value());
    EXPECT_EQ(*idx_x, 0u);

    auto idx_y = schema.field_index("y");
    ASSERT_TRUE(idx_y.has_value());
    EXPECT_EQ(*idx_y, 1u);

    auto idx_z = schema.field_index("z");
    EXPECT_FALSE(idx_z.has_value());
}

TEST(StreamSchemaTest, CompatibleWithSameSchema) {
    StreamSchema s1;
    s1.add_field("a", TypeKind::I32);
    s1.add_field("b", TypeKind::String);

    StreamSchema s2;
    s2.add_field("a", TypeKind::I32);
    s2.add_field("b", TypeKind::String);

    EXPECT_TRUE(s1.is_compatible_with(s2));
    EXPECT_TRUE(s2.is_compatible_with(s1));
}

TEST(StreamSchemaTest, IncompatibleDifferentCount) {
    StreamSchema s1;
    s1.add_field("a", TypeKind::I32);

    StreamSchema s2;
    s2.add_field("a", TypeKind::I32);
    s2.add_field("b", TypeKind::String);

    EXPECT_FALSE(s1.is_compatible_with(s2));
}

TEST(StreamSchemaTest, IncompatibleDifferentName) {
    StreamSchema s1;
    s1.add_field("a", TypeKind::I32);

    StreamSchema s2;
    s2.add_field("b", TypeKind::I32);

    EXPECT_FALSE(s1.is_compatible_with(s2));
}

TEST(StreamSchemaTest, IncompatibleDifferentType) {
    StreamSchema s1;
    s1.add_field("a", TypeKind::I32);

    StreamSchema s2;
    s2.add_field("a", TypeKind::I64);

    EXPECT_FALSE(s1.is_compatible_with(s2));
}

TEST(StreamSchemaTest, CompatibleEmptySchema) {
    StreamSchema s1;
    StreamSchema s2;
    EXPECT_TRUE(s1.is_compatible_with(s2));
}

}  // namespace
}  // namespace xtream
