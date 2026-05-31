#pragma once

#include <memory>
#include <string_view>
#include <vector>

#include "core/event.h"
#include "core/record.h"
#include "operators/physical/collection_source_physical_operator.h"

namespace extream {

class SourceLogicalOperatorImpl {
public:
    using Data = std::vector<Event<Record>>;

    SourceLogicalOperatorImpl() = default;
    explicit SourceLogicalOperatorImpl(Data data) : data_(std::move(data)) {}

    std::string_view type_name() const { return "Source"; }
    const Data& data() const { return data_; }

    std::shared_ptr<PhysicalOperator> compile() const {
        return std::make_shared<CollectionSourcePhysicalOperator>(data_);
    }

private:
    Data data_;
};

}  // namespace extream
