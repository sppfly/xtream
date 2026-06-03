#include "engine/slot.h"

namespace xtream {

Slot::~Slot() {
    if (thread_.joinable()) {
        thread_.join();
    }
}

void Slot::assign(Pipeline pipeline) {
    pipeline_ = std::unique_ptr<Pipeline>(new Pipeline(std::move(pipeline)));
}

void Slot::start() {
    if (running_) {
        return;
    }
    running_ = true;
    thread_ = std::thread([this]() { run(); });
}

void Slot::stop() {
    if (pipeline_) {
        pipeline_->stop();
    }
    if (thread_.joinable()) {
        thread_.join();
    }
    running_ = false;
}

void Slot::run() {
    pipeline_->run();
    running_ = false;
}

}  // namespace xtream
