#pragma once

#include <atomic>
#include <memory>
#include <thread>

#include "engine/pipeline.h"

namespace xtream {

class Slot {
public:
    Slot() = default;
    ~Slot();

    Slot(const Slot&) = delete;
    Slot& operator=(const Slot&) = delete;

    void assign(Pipeline pipeline);
    void start();
    void stop();
    bool is_running() const { return running_; }

private:
    void run();

    std::unique_ptr<Pipeline> pipeline_;
    std::thread thread_;
    std::atomic<bool> running_{false};
};

}  // namespace xtream
