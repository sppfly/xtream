#pragma once

#include <cstdint>

#include "core/id.h"

namespace xtream {

enum class EdgePartition : uint8_t { Forward, Keyed, Broadcast };

class StreamEdge {
public:
    StreamEdge(EdgeId id, OperatorId source, OperatorId target, EdgePartition partition)
        : id_(id), source_(source), target_(target), partition_(partition) {}

    EdgeId id() const { return id_; }
    OperatorId source() const { return source_; }
    OperatorId target() const { return target_; }
    EdgePartition partition() const { return partition_; }

private:
    EdgeId id_;
    OperatorId source_;
    OperatorId target_;
    EdgePartition partition_;
};

}  // namespace xtream
