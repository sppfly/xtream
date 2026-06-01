#pragma once

#include <cassert>
#include <memory>
#include <string>
#include <vector>

#include "core/stream_schema.h"
#include "core/types/types.h"

namespace xtream {

class FieldValue {
public:
    FieldValue() : kind_(TypeKind::I32), int_storage_(0), float_storage_(0.0) {}

    static FieldValue make_i32(i32 v) {
        FieldValue fv;
        fv.kind_ = TypeKind::I32;
        fv.int_storage_ = v.raw();
        return fv;
    }
    static FieldValue make_i64(i64 v) {
        FieldValue fv;
        fv.kind_ = TypeKind::I64;
        fv.int_storage_ = v.raw();
        return fv;
    }
    static FieldValue make_u32(u32 v) {
        FieldValue fv;
        fv.kind_ = TypeKind::U32;
        fv.int_storage_ = static_cast<int64_t>(v.raw());
        return fv;
    }
    static FieldValue make_u64(u64 v) {
        FieldValue fv;
        fv.kind_ = TypeKind::U64;
        fv.int_storage_ = static_cast<int64_t>(v.raw());
        return fv;
    }
    static FieldValue make_f32(f32 v) {
        FieldValue fv;
        fv.kind_ = TypeKind::F32;
        fv.float_storage_ = static_cast<double>(v.raw());
        return fv;
    }
    static FieldValue make_f64(f64 v) {
        FieldValue fv;
        fv.kind_ = TypeKind::F64;
        fv.float_storage_ = v.raw();
        return fv;
    }
    static FieldValue make_string(std::string v) {
        FieldValue fv;
        fv.kind_ = TypeKind::String;
        fv.string_storage_ = std::move(v);
        return fv;
    }

    TypeKind kind() const { return kind_; }

    i32 as_i32() const {
        assert(kind_ == TypeKind::I32);
        return i32(static_cast<int32_t>(int_storage_));
    }
    i64 as_i64() const {
        assert(kind_ == TypeKind::I64);
        return i64(int_storage_);
    }
    u32 as_u32() const {
        assert(kind_ == TypeKind::U32);
        return u32(static_cast<uint32_t>(int_storage_));
    }
    u64 as_u64() const {
        assert(kind_ == TypeKind::U64);
        return u64(static_cast<uint64_t>(int_storage_));
    }
    f32 as_f32() const {
        assert(kind_ == TypeKind::F32);
        return f32(static_cast<float>(float_storage_));
    }
    f64 as_f64() const {
        assert(kind_ == TypeKind::F64);
        return f64(float_storage_);
    }
    const std::string& as_string() const {
        assert(kind_ == TypeKind::String);
        return string_storage_;
    }

private:
    TypeKind kind_;
    int64_t int_storage_;
    double float_storage_;
    std::string string_storage_;
};

class Record {
public:
    using SchemaPtr = std::shared_ptr<const StreamSchema>;

    explicit Record(SchemaPtr schema) : schema_(std::move(schema)) {}

    SchemaPtr schema() const { return schema_; }

    size_t field_count() const { return fields_.size(); }

    void add_field(FieldValue value) { fields_.push_back(std::move(value)); }

    const FieldValue& field(size_t index) const {
        assert(index < fields_.size());
        return fields_[index];
    }

    const FieldValue& field(std::string_view name) const {
        auto idx = schema_->field_index(name);
        assert(idx.has_value());
        return fields_[*idx];
    }

private:
    SchemaPtr schema_;
    std::vector<FieldValue> fields_;
};

}  // namespace xtream
