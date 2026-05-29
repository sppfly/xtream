#include "core/record.h"

#include <gtest/gtest.h>

#include <memory>

namespace extream {
namespace {

static std::shared_ptr<StreamSchema> make_schema() {
    auto schema = std::make_shared<StreamSchema>();
    schema->add_field("id", TypeKind::I64);
    schema->add_field("name", TypeKind::String);
    schema->add_field("score", TypeKind::F64);
    return schema;
}

TEST(RecordTest, ConstructWithSchema) {
    auto schema = make_schema();
    Record record(schema);
    EXPECT_EQ(record.schema(), schema);
}

TEST(RecordTest, AddAndGetFieldsByIndex) {
    auto schema = make_schema();
    Record record(schema);
    record.add_field(FieldValue::make_i64(i64(42)));
    record.add_field(FieldValue::make_string("test"));
    record.add_field(FieldValue::make_f64(f64(3.14)));

    EXPECT_EQ(record.field_count(), 3u);
    EXPECT_EQ(record.field(0).as_i64().raw(), 42);
    EXPECT_EQ(record.field(1).as_string(), "test");
    EXPECT_DOUBLE_EQ(record.field(2).as_f64().raw(), 3.14);
}

TEST(RecordTest, GetFieldByName) {
    auto schema = make_schema();
    Record record(schema);
    record.add_field(FieldValue::make_i64(i64(100)));
    record.add_field(FieldValue::make_string("hello"));
    record.add_field(FieldValue::make_f64(f64(2.5)));

    EXPECT_EQ(record.field("id").as_i64().raw(), 100);
    EXPECT_EQ(record.field("name").as_string(), "hello");
    EXPECT_DOUBLE_EQ(record.field("score").as_f64().raw(), 2.5);
}

TEST(RecordTest, I32Field) {
    auto schema = std::make_shared<StreamSchema>();
    schema->add_field("val", TypeKind::I32);
    Record record(schema);
    record.add_field(FieldValue::make_i32(i32(7)));

    EXPECT_EQ(record.field("val").kind(), TypeKind::I32);
    EXPECT_EQ(record.field("val").as_i32().raw(), 7);
}

TEST(RecordTest, U32Field) {
    auto schema = std::make_shared<StreamSchema>();
    schema->add_field("val", TypeKind::U32);
    Record record(schema);
    record.add_field(FieldValue::make_u32(u32(42u)));

    EXPECT_EQ(record.field("val").kind(), TypeKind::U32);
    EXPECT_EQ(record.field("val").as_u32().raw(), 42u);
}

TEST(RecordTest, U64Field) {
    auto schema = std::make_shared<StreamSchema>();
    schema->add_field("val", TypeKind::U64);
    Record record(schema);
    record.add_field(FieldValue::make_u64(u64(999u)));

    EXPECT_EQ(record.field("val").kind(), TypeKind::U64);
    EXPECT_EQ(record.field("val").as_u64().raw(), 999u);
}

TEST(RecordTest, F32Field) {
    auto schema = std::make_shared<StreamSchema>();
    schema->add_field("val", TypeKind::F32);
    Record record(schema);
    record.add_field(FieldValue::make_f32(f32(1.5f)));

    EXPECT_EQ(record.field("val").kind(), TypeKind::F32);
    EXPECT_FLOAT_EQ(record.field("val").as_f32().raw(), 1.5f);
}

TEST(RecordTest, FieldHasCorrectKind) {
    auto fv = FieldValue::make_i64(i64(10));
    EXPECT_EQ(fv.kind(), TypeKind::I64);

    auto fv2 = FieldValue::make_string("text");
    EXPECT_EQ(fv2.kind(), TypeKind::String);

    auto fv3 = FieldValue::make_f64(f64(1.0));
    EXPECT_EQ(fv3.kind(), TypeKind::F64);
}

}  // namespace
}  // namespace extream
