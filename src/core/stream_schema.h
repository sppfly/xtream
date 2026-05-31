#pragma once

#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <vector>

#include "core/types/types.h"

namespace extream {

enum class TypeKind : uint8_t { I32, I64, U32, U64, F32, F64, String };

class StreamSchema {
public:
    struct Field {
        std::string name;
        TypeKind type;

        bool operator==(const Field& other) const {
            return name == other.name && type == other.type;
        }
    };

    void add_field(std::string name, TypeKind type) { fields_.push_back({std::move(name), type}); }

    const std::vector<Field>& fields() const { return fields_; }
    size_t field_count() const { return fields_.size(); }

    std::optional<size_t> field_index(std::string_view name) const {
        for (auto [i, f] : fields_ | std::views::enumerate) {
            if (f.name == name) return i;
        }
        return std::nullopt;
    }

    bool is_compatible_with(const StreamSchema& other) const {
        return std::ranges::equal(fields_, other.fields_);
    }

private:
    std::vector<Field> fields_;
};

}  // namespace extream
