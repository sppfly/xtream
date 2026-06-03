#include "engine/slot_engine.h"

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <thread>
#include <vector>

#include "core/types/types.h"
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

TEST(SlotEngineTest, SingleQuery) {
    std::vector<Event<Record>> collected;
    int counter = 0;

    auto graph = DataflowGraphBuilder()
                     .source([&counter]() -> std::optional<Event<Record>> {
                         ++counter;
                         if (counter > 5) {
                             return std::nullopt;
                         }
                         return Event<Record>(make_record(counter), i64(counter));
                     })
                     .sink([&collected](Event<Record>& e) { collected.push_back(e); })
                     .build();

    SlotEngine engine(1_usize);
    engine.submit(std::move(graph));

    std::thread engine_thread([&engine]() { engine.execute(); });

    engine_thread.join();

    ASSERT_EQ(collected.size(), 5u);
}

TEST(SlotEngineTest, MultipleQueries) {
    std::vector<std::vector<Event<Record>>> results(3);

    auto create_graph = [&](int query_id, std::vector<Event<Record>>& collected) {
        return DataflowGraphBuilder()
            .source([query_id, count = 0]() mutable -> std::optional<Event<Record>> {
                ++count;
                if (count > 5) {
                    return std::nullopt;
                }
                return Event<Record>(make_record(query_id * 100 + count), i64(count));
            })
            .sink([&collected](Event<Record>& e) { collected.push_back(e); })
            .build();
    };

    SlotEngine engine(3_usize);
    for (int i = 0; i < 3; ++i) {
        engine.submit(create_graph(i, results[i]));
    }

    std::thread engine_thread([&engine]() { engine.execute(); });

    engine_thread.join();

    for (auto& collected : results) {
        EXPECT_GT(collected.size(), 0u);
    }
}

TEST(SlotEngineTest, CancelExecution) {
    std::atomic<int> counter{0};

    auto graph = DataflowGraphBuilder()
                     .source([&counter]() -> std::optional<Event<Record>> {
                         ++counter;
                         return Event<Record>(make_record(counter.load()), i64(counter.load()));
                     })
                     .sink([](Event<Record>&) {})
                     .build();

    SlotEngine engine(1_usize);
    engine.submit(std::move(graph));

    std::thread engine_thread([&engine]() { engine.execute(); });

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    engine.cancel();

    engine_thread.join();

    EXPECT_GT(counter.load(), 0);
}

}  // namespace
}  // namespace xtream
