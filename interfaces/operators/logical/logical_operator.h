#pragma once

#include <memory>
#include <string>
#include <string_view>

#include "core/id.h"

namespace extream {

class LogicalOperator {
public:
    template <typename T>
    LogicalOperator(OperatorId id, std::string name, u64 parallelism, T impl)
        : self_(std::make_shared<Model<T>>(std::move(impl))),
          id_(id),
          name_(std::move(name)),
          parallelism_(parallelism) {}

    OperatorId id() const { return id_; }
    const std::string& name() const { return name_; }
    u64 parallelism() const { return parallelism_; }
    std::string_view type_name() const { return self_->type_name(); }

private:
    struct Concept {
        virtual ~Concept() = default;
        virtual std::string_view type_name() const = 0;
    };

    template <typename T>
    struct Model : Concept {
        T impl;
        explicit Model(T i) : impl(std::move(i)) {}
        std::string_view type_name() const override { return impl.type_name(); }
    };

    std::shared_ptr<const Concept> self_;
    OperatorId id_;
    std::string name_;
    u64 parallelism_;
};

}  // namespace extream
