#include "engine/slot.h"

namespace xtream {

void Slot::assign(Pipeline pipeline) {
    pipeline_ = std::unique_ptr<Pipeline>(new Pipeline(std::move(pipeline)));
}

void Slot::start() {
    if (running_) {
        return;
    }
    running_ = true;
    thread_ = std::jthread([this]() { run(); });
}

void Slot::stop() {
    if (pipeline_) {
        pipeline_->stop();
    }
    thread_.request_stop();
    thread_.join();
    running_ = false;
}

void Slot::run() {
    pipeline_->run();
    running_ = false;
}

}  // namespace xtream
