#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

#include "core/types/types.h"

namespace xtream {

template <typename T>
class Event {
public:
    Event(T payload, i64 timestamp) : payload_(std::move(payload)), timestamp_(timestamp) {}

    const T& payload() const { return payload_; }
    i64 timestamp() const { return timestamp_; }

    void set_metadata(std::string_view key, std::string value) {
        metadata_[std::string(key)] = std::move(value);
    }

    std::optional<std::string_view> metadata(std::string_view key) const {
        auto it = metadata_.find(std::string(key));
        if (it != metadata_.end()) {
            return it->second;
        }
        return std::nullopt;
    }

private:
    T payload_;
    i64 timestamp_;
    std::unordered_map<std::string, std::string> metadata_;
};

}  // namespace xtream
