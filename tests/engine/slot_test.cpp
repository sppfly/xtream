#include "engine/slot.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

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

TEST(SlotTest, SinglePipelineExecution) {
    std::vector<Event<Record>> collected;
    int counter = 0;

    auto graph =
        DataflowGraphBuilder()
            .source([&counter]() -> std::optional<Event<Record>> {
                ++counter;
                if (counter > 5) {
                    return std::nullopt;
                }
                return Event<Record>(make_record(counter), u64(static_cast<uint64_t>(counter)));
            })
            .sink([&collected](Event<Record>& e) { collected.push_back(e); })
            .build();

    Pipeline pipeline(std::move(graph));
    Slot slot;
    slot.assign(std::move(pipeline));
    slot.start();

    while (slot.is_running()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    ASSERT_EQ(collected.size(), 5u);
    for (size_t i = 0; i < collected.size(); ++i) {
        EXPECT_EQ(collected[i].payload().field(0).as_i32().raw(), static_cast<int>(i + 1));
    }
}

TEST(SlotTest, StopBeforeCompletion) {
    std::vector<Event<Record>> collected;
    std::atomic<int> counter{0};

    auto graph = DataflowGraphBuilder()
                     .source([&counter]() -> std::optional<Event<Record>> {
                         ++counter;
                         return Event<Record>(make_record(counter.load()),
                                              u64(static_cast<uint64_t>(counter.load())));
                     })
                     .sink([&collected](Event<Record>& e) { collected.push_back(e); })
                     .build();

    Pipeline pipeline(std::move(graph));
    Slot slot;
    slot.assign(std::move(pipeline));
    slot.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    slot.stop();

    EXPECT_FALSE(slot.is_running());
    EXPECT_GT(collected.size(), 0u);
}

}  // namespace
}  // namespace xtream
