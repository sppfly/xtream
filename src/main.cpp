#include <iostream>
#include <memory>
#include <optional>

#include "core/event.h"
#include "core/record.h"
#include "core/types/window_spec.h"
#include "engine/execution_graph.h"
#include "engine/slot_engine.h"
#include "graph/dataflow_graph_builder.h"

using namespace xtream;

auto make_record(int v) -> Record {
    auto schema = std::make_shared<StreamSchema>();
    schema->add_field("value", TypeKind::I32);
    auto record = Record(schema);
    record.add_field(FieldValue::make_i32(i32(v)));
    return record;
}

int main() {
    int counter = 0;

    auto graph =
        DataflowGraphBuilder()
            .source([&counter]() -> std::optional<Event<Record>> {
                ++counter;
                if (counter > 15) {
                    return std::nullopt;
                }
                return Event<Record>(make_record(counter), u64(static_cast<uint64_t>(counter)));
            })
            .map([](Event<Record>& e) -> Event<Record> {
                int raw = e.payload().field(0).as_i32().raw();
                return Event<Record>(make_record(raw * 2), e.timestamp());
            })
            .window(count_tumbling_window(5_usize))
            .reduce([](const std::vector<Event<Record>>& events) -> Event<Record> {
                int sum = 0;
                for (const auto& e : events) {
                    sum += e.payload().field(0).as_i32().raw();
                }
                return Event<Record>(make_record(sum), events.back().timestamp());
            })
            .sink([](Event<Record>& e) {
                std::cout << "window sum: " << e.payload().field(0).as_i32().raw()
                          << " (ts=" << e.timestamp().raw() << ")" << std::endl;
            })
            .build();

    auto pipelines = ExecutionGraph::build(graph);
    std::cout << "Pipelines: " << pipelines.size() << std::endl;

    SlotEngine engine(usize{pipelines.size()});
    for (auto& p : pipelines) {
        engine.submit(std::move(p));
    }
    engine.execute();

    std::cout << "Done." << std::endl;
    return 0;
}
