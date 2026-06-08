#pragma once

#include <condition_variable>
#include <mutex>
#include <optional>
#include <queue>

#include "core/event.h"
#include "core/record.h"

namespace xtream {

class InputChannel {
public:
    InputChannel() = default;

    InputChannel(const InputChannel&) = delete;
    InputChannel& operator=(const InputChannel&) = delete;
    InputChannel(InputChannel&&) = delete;
    InputChannel& operator=(InputChannel&&) = delete;

    void write(Event<Record> event) {
        {
            std::lock_guard lock(mutex_);
            queue_.push(std::move(event));
        }
        cv_.notify_one();
    }

    void close() {
        {
            std::lock_guard lock(mutex_);
            closed_ = true;
        }
        cv_.notify_all();
    }

    std::optional<Event<Record>> read() {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [this] { return !queue_.empty() || closed_; });
        if (queue_.empty()) {
            return std::nullopt;
        }
        auto event = std::move(queue_.front());
        queue_.pop();
        return event;
    }

    bool is_done() const {
        std::lock_guard lock(mutex_);
        return closed_ && queue_.empty();
    }

private:
    mutable std::mutex mutex_;
    std::condition_variable cv_;
    std::queue<Event<Record>> queue_;
    bool closed_{false};
};

}  // namespace xtream
