#pragma once

#include <cstdint>
#include <string>

#include "core/id.h"

namespace extream {

struct OperatorTag {};
using OperatorId = Id<OperatorTag>;

class OperatorDescriptor {
public:
    OperatorDescriptor(OperatorId id, std::string name, std::string type, size_t parallelism)
        : id_(id), name_(std::move(name)), type_(std::move(type)), parallelism_(parallelism) {}

    OperatorId id() const { return id_; }
    const std::string& name() const { return name_; }
    const std::string& type() const { return type_; }
    size_t parallelism() const { return parallelism_; }

private:
    OperatorId id_;
    std::string name_;
    std::string type_;
    size_t parallelism_;
};

}  // namespace extream
