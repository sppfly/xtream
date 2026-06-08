#include "engine/slot_manager.h"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>
#include <vector>

#include "core/types/types.h"
#include "engine/pipeline.h"
#include "graph/dataflow_graph_builder.h"
#include "operators/logical/sink_logical_operator.h"
#include "operators/logical/source_logical_operator.h"

namespace xtream {
namespace {

auto make_record(int v) -> Record {
    auto schema = std::make_shared<StreamSchema>();
    schema->add_field("value", TypeKind::I32);
    auto record = Record(schema);
    record.add_field(FieldValue::make_i32(i32(v)));
    return record;
}

TEST(SlotManagerTest, CreateMultipleSlots) {
    SlotManager manager(3_usize);
    EXPECT_EQ(manager.slot_count(), 3_usize);
}

TEST(SlotManagerTest, AssignAndStartMultipleSlots) {
    std::vector<std::vector<Event<Record>>> results(3);

    auto create_graph = [&](int slot_id, std::vector<Event<Record>>& collected) {
        return DataflowGraphBuilder()
            .source([slot_id, count = 0]() mutable -> std::optional<Event<Record>> {
                ++count;
                if (count > 5) {
                    return std::nullopt;
                }
                return Event<Record>(make_record(slot_id * 100 + count),
                                     u64(static_cast<uint64_t>(count)));
            })
            .sink([&collected](Event<Record>& e) { collected.push_back(e); })
            .build();
    };

    SlotManager manager(3_usize);
    for (int i = 0; i < 3; ++i) {
        Pipeline pipeline(create_graph(i, results[i]));
        manager.get_slot(usize{static_cast<uint64_t>(i)}).assign(std::move(pipeline));
    }

    manager.start_all();

    for (usize i{0_usize}; i < manager.slot_count(); ++i) {
        while (manager.get_slot(i).is_running()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }

    for (auto& collected : results) {
        EXPECT_GT(collected.size(), 0u);
    }
}

}  // namespace
}  // namespace xtream
