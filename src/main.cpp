#include <iostream>
#include <memory>
#include <optional>

#include "core/event.h"
#include "core/record.h"
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

auto value_of(const Event<Record>& e) -> int {
    return e.payload().field(0).as_i32().raw();
}

int main() {
    int counter = 0;

    auto graph = DataflowGraphBuilder()
                     .source([&counter]() -> std::optional<Event<Record>> {
                         ++counter;
                         if (counter > 10) {
                             return std::nullopt;
                         }
                         return Event<Record>(make_record(counter), i64(counter));
                     })
                     .map([](Event<Record>& e) -> Event<Record> {
                         return Event<Record>(make_record(value_of(e) * 2), e.timestamp());
                     })
                     .filter([](Event<Record>& e) -> bool { return value_of(e) > 10; })
                     .sink([](Event<Record>& e) {
                         std::cout << "output: " << value_of(e) << " (ts=" << e.timestamp().raw()
                                   << ")" << std::endl;
                     })
                     .build();

    std::cout << "Starting SlotEngine with 1 slot..." << std::endl;

    SlotEngine engine(1_usize);
    engine.submit(std::move(graph));
    engine.execute();

    std::cout << "Done." << std::endl;
    return 0;
}
