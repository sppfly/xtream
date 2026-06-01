# Development Plan — xtream Stream Computing Platform

## Strategy

- **Single-node first**: each engine is fully functional on a single node before going distributed
- **Extension points designed from the start**: interfaces (engine, placement) are abstract so concrete implementations can be swapped
- **Every phase delivers testable code**: no phase produces only docs or stubs

---

## Phase 1: Infrastructure ✅ DONE

**Goal**: Strict basic types + Logger + project skeleton with full tooling.

### Deliverables

| Task | Files |
|------|-------|
| Project skeleton | `.clang-format`, `.gitignore`, `CMakeLists.txt`, `CMakePresets.json`, `src/CMakeLists.txt`, `tests/CMakeLists.txt` |
| Strict signed integers | `src/core/types/int.h` — `StrictInt<T>` template + `i8`, `i16`, `i32`, `i64` |
| Strict unsigned integers | `src/core/types/uint.h` — `u8`, `u16`, `u32`, `u64` |
| Strict floats | `src/core/types/float.h` — `f32`, `f64` |
| Unified type header | `src/core/types/types.h` — re-exports all basic types |
| Logger | `src/util/log/logger.h`, `logger.cpp` — 5 levels, runtime filtering, thread-safe, `std::source_location` |
| GoogleTest integration | FetchContent in `tests/CMakeLists.txt` |
| Type unit tests | `tests/core/types/int_test.cpp`, `uint_test.cpp`, `float_test.cpp` |
| Logger unit tests | `tests/util/log/logger_test.cpp` |

### Design Decisions Made

- **Single `StrictInt<T>` template** in `int.h` reused via aliases in `uint.h` (avoid duplication)
- **No macros**: Logger uses `std::source_location` default parameter, no LOG_INFO macros
- **`if constexpr (std::is_signed_v<T>)`** for signed-only overflow asserts; unsigned wraps freely
- **Toolchain auto-detection**: CMakeLists.txt finds clang++, ccache, mold automatically
- **`compile_commands.json`** generated in build/ for LSP support
- **`ninja -C build format`** target for clang-format auto-fix

### Verification

```bash
cmake -B build -S .
ninja -C build
cd build && ctest --output-on-failure
```

---

## Phase 2: Core Data Model ✅ DONE

**Depends on**: Phase 1

### Deliverables

| Task | Files |
|------|-------|
| `Event<T>` | `src/core/event.h` — payload + timestamp + metadata (opaque map) |
| `Watermark` | `src/core/watermark.h` — timestamp + source-id, comparison, min_timestamp |
| `StreamSchema` | `src/core/stream_schema.h` — ordered list of (field-name, type-id), compatibility check |
| `Record` | `src/core/record.h` — `FieldValue` (discriminated union) + `Record` (schema-backed row) |

### Design Decisions Made

- `Event<T>` payload stored by value; `set_metadata()` / `metadata()` for key-value attachments
- `Watermark::min_timestamp()` utility for taking min across multiple input watermarks
- `TypeKind` enum uses `uint8_t` underlying type (can't use `u8` — enum requires integral type)
- `FieldValue` is a simple tagged union: stores scalar in `int64_t`/`double`, strings in `std::string`
- `Record` holds a `shared_ptr<const StreamSchema>`; fields accessed by index or name

### Test Coverage (32 new tests)

- Event: construction, copy/move, metadata CRUD, timestamp ordering, different payload types
- Watermark: equality, ordering (by timestamp then source_id), min_timestamp, copy
- StreamSchema: empty/compatible/incompatible by count/name/type, field_index lookup
- Record: field access by index and name, all type kinds (i32/u32/u64/f32/f64/string)

---

## Phase 3: Dataflow Graph (DAG) ✅ DONE

**Depends on**: Phase 2

### Deliverables

| Task | Files |
|------|-------|
| `OperatorDescriptor` | `src/graph/operator_descriptor.h` — id, name, type, parallelism |
| `StreamEdge` + `EdgePartition` | `src/graph/stream_edge.h` — source/target operator id, Forward/Keyed/Broadcast |
| `DataflowGraph` (immutable) | `src/graph/dataflow_graph.h` — query operators/edges, `sources_of` / `targets_of` |
| `DataflowGraphBuilder` | `src/graph/dataflow_graph_builder.h` — fluent builder, `validate()` + `build()` |

### Design Decisions Made

- **Builder pattern**: `DataflowGraphBuilder` returns `OperatorId` / `EdgeId` (value types, no dangling reference risk on vector realloc). `build()` validates then produces immutable `DataflowGraph`.
- **Cycle detection**: Kahn's algorithm (in-degree based) — O(V+E), clear error message on cycle.
- **Schema validation deferred to Phase 4**: operators don't carry schema yet, so schema compat is a placeholder for now.
- **`OperatorTag`/`EdgeTag`** defined alongside descriptors, providing separate `OperatorId`/`EdgeId` types via `Id<Tag>`.

### Test Coverage (16 new tests)

- Build/validate: empty graph rejected, single op ok
- Topology: linear, fan-out, fan-in
- Cycles: 3-node cycle, self-loop, 2-node cycle — all rejected
- Query: find by id, find by name, not found = nullptr, `sources_of`/`targets_of`

---

## Phase 4: Operator Interfaces + Simple Operators ✅ DONE

**Depends on**: Phase 3

### Deliverables

| Task | Files |
|------|-------|
| Function concepts | `interfaces/functions/{map,filter,source,sink}_function.h` — C++20 concepts |
| LogicalOperator | `interfaces/operators/logical/logical_operator.h` — type-erased wrapper |
| 4 LogicalOperatorImpl | `interfaces/operators/logical/{map,filter,source,sink}_logical_operator.h` |
| PhysicalOperator | `interfaces/operators/physical/physical_operator.h` — lifecycle + shared_ptr chain |
| 4 PhysicalOperator | `src/operators/physical/{map,filter,collection_source,collect_sink}_physical_operator.h` |
| OperatorDescriptor removed | `DataflowGraph` now holds `vector<LogicalOperator>` + `vector<StreamEdge>` |

### Design Decisions Made

- **PhysicalOperator uses shared_ptr chain**: operators hold `shared_ptr<PhysicalOperator> next_`, call `next_->execute()` directly. No OperatorContext for routing.
- **Lifecycle without ctx**: `setup()`, `open()`, `execute(Event<Record>&)`, `close()`, `terminate()` — all parameterless except execute. OperatorContext deferred to Phase 6.
- **Source/Sink logical operators are stubs**: `SourceLogicalOperatorImpl` and `SinkLogicalOperatorImpl` carry no function payload. They'll be filled when the compilation bridge is built.
- **Function concepts exist but aren't enforced on LogicalOperator**: `MapLogicalOperatorImpl` uses `std::function` directly, not the `MapFunction` concept. Concept enforcement is a future improvement.

### What Was NOT Done (deferred)

- **OperatorContext**: runtime context for watermark, state, config, metrics → Phase 6
- **Compilation bridge**: LogicalOperator → PhysicalOperator → Phase 5
- **Concept enforcement on LogicalOperator**: LogicalOperatorImpl uses `std::function`, not concepts

### Test Coverage (158 total, 26 new in Phase 4)

- Function concepts: lambda/functor satisfies each concept, wrong signatures rejected
- LogicalOperator: construction, type_name, id, parallelism, stored in vector
- PhysicalOperator: lifecycle ordering, map transforms, filter drops, chain execution

---

## Phase 5: Compilation Bridge + Simple Execution

**Depends on**: Phase 4

Current operators (Map, Filter, Source, Sink) are all stateless and cascadable. They run
in a single chain on one thread. No slots, no threads, no placement needed yet.

### Deliverables

| Task | Files |
|------|-------|
| Compilation bridge | `src/engine/compiler.h` — `DataflowGraph` → `PhysicalOperator` chain |
| LogicalOperator.compile() | Add virtual `compile()` to `LogicalOperator::Concept` |
| Fill Source/Sink stubs | `SourceLogicalOperatorImpl` and `SinkLogicalOperatorImpl` store actual functions |
| Simple execution | `src/engine/simple_engine.h` — compile + trigger source, chain runs end-to-end |
| Parallelism > 1 | Chain duplication: operator with parallelism N produces N chains |

### Compilation Bridge Design

```
DataflowGraph
    │ topological_order()
    ▼
for each LogicalOperator:
    physical = logical.compile()    // virtual dispatch → MapLogicalOperatorImpl.compile() → MapPhysicalOperator
    link to previous via set_next()

source->open()
source->execute(dummy)   // triggers whole chain
source->close()
```

### Parallelism > 1

For operators with parallelism > 1, the compiler duplicates the downstream chain:
```
Source (parallelism=1) → Map (parallelism=3) → Sink (parallelism=1)
                              │
                    ┌─────────┼─────────┐
                    ▼         ▼         ▼
              MapChain1  MapChain2  MapChain3
                    │         │         │
                    └─────────┼─────────┘
                              ▼
                            Sink
```

Source routes events round-robin to N Map chains. Sink merges input from all chains.

### Design Constraints

- Single-threaded: all operators run on the calling thread
- No OperatorContext yet (deferred to Phase 6 for watermark/state)
- No inter-task communication (no queues, no threads)
- Compilation is pure: DataflowGraph in, PhysicalOperator chain out

### Tests

- Compile single-operator graph (source → sink)
- Compile multi-operator graph (source → map → filter → sink)
- Parallelism > 1: source → map(3) → sink produces correct merged output
- Graph with fan-out compiles correctly

---

## Phase 6: Pipeline — Single-Threaded Execution

**Depends on**: Phase 5

Current state: `Compiler::compile(graph)` produces a `PhysicalOperator` chain, but lifecycle
has to be managed manually (open → loop execute → close).

### Deliverables

| Task | Files |
|------|-------|
| `Pipeline` class | `src/engine/pipeline.h` — wraps a `DataflowGraph`, compiles + runs lifecycle automatically |

### Pipeline API

```cpp
class Pipeline {
    // Construct and compile
    Pipeline(DataflowGraph graph);
    
    // Run the entire pipeline: setup → open → execute all source events → close → terminate
    void run();
};
```

### Design Constraints

- No threading: everything runs on the calling thread
- No slots, no placement, no parallelism
- Chain is compiled from `DataflowGraph` using `Compiler`
- `run()` wraps the full lifecycle: `setup()` → `open()` → `execute(N)` → `close()` → `terminate()`

### Tests

- Pipeline::run() executes a Source → Sink graph end-to-end
- Pipeline::run() executes a Source → Map → Filter → Sink graph correctly

---

## Phase 7: Engine A — Fixed-Slot (Deferred)

**When**: Single-threaded `Pipeline` becomes a bottleneck — e.g., multiple independent
pipelines, or stateful operators needing separate threads.

**What it will do**:
- `ExecutionEngine` interface: `submit(JobGraph)`, `execute()`, `cancel()`
- `Slot` — a thread that runs one task at a time
- `SlotManager` — pool of N slots
- `SlotEngine` — assigns compiled `PhysicalOperator` chains to slots
- `GreedyPlacer` — first-fit assignment of tasks to slots

The `Pipeline` class (Phase 6) becomes a degenerate case of SlotEngine with one slot.

---

## Phase 8: Engine B — Task-Stealing (Deferred)

**When**: Engine A's fixed-slot model proves too rigid. Task-stealing improves resource
utilization when tasks have uneven workloads.

**What it will do**:
- `StealingEngine` — implements `ExecutionEngine`
- `TaskQueue` — thread-safe MPSC queue of pending tasks
- `Executor` — worker thread that pulls and runs tasks
- `PartitionGuard` — ensures only one executor processes a given partition at a time
  (preventing out-of-order output when multiple executors hit the same partition)

---

## Phase 9: Advanced Operators

**Depends on**: Phase 5

Following the same two-layer split as Phase 4. Operators defined here; watermark/trigger driving
mechanism deferred to Phase 9.

### Logical Layer (User-Facing, Pure Description)

| Task | Files |
|------|-------|
| `KeyByLogicalOperator` | `interfaces/operators/logical/keyby_logical_operator.h` — holds key extractor function |
| `TumblingWindowLogicalOperator` | `interfaces/operators/logical/tumbling_window_logical_operator.h` — window size + aggregate |
| `SlidingWindowLogicalOperator` | `interfaces/operators/logical/sliding_window_logical_operator.h` — size, slide, aggregate |
| `ReduceLogicalOperator` | `interfaces/operators/logical/reduce_logical_operator.h` — associative reduce function |
| `JoinLogicalOperator` | `interfaces/operators/logical/join_logical_operator.h` — key selector, window spec |

### Physical Layer (Runtime, Lifecycle-Managed)

| Task | Files |
|------|-------|
| `KeyByPhysicalOperator` | `src/operators/physical/keyby_physical_operator.h` — routes events to output partition |
| `TumblingWindowPhysicalOperator` | `src/operators/physical/tumbling_window_physical_operator.h` — stateful window agg |
| `SlidingWindowPhysicalOperator` | `src/operators/physical/sliding_window_physical_operator.h` — overlapping window state |
| `ReducePhysicalOperator` | `src/operators/physical/reduce_physical_operator.h` — incremental reduce with state |
| `JoinPhysicalOperator` | `src/operators/physical/join_physical_operator.h` — time-windowed join, dual input |

### Design Constraints

- `KeyBy` changes edge partitioning to `Keyed` — affects how the engine routes events
- Window operators hold state in-memory; trigger/watermark driving added in Phase 9
- `JoinPhysicalOperator` is a non-cascadable operator (two-input, stateful)

### Tests

- Tumbling window buffers events correctly
- Sliding window with slide < size produces overlapping windows
- Join pairs events with matching keys within time window
- Late events are handled once watermark is integrated (Phase 9)

---

## Phase 10: Watermark Propagation + Trigger Framework

**Depends on**: Phase 9

### Deliverables

| Task | Files |
|------|-------|
| `Trigger` abstract class | `src/runtime/trigger.h` — `onEvent()`, `onWatermark()`, returns `TriggerResult` |
| `TriggerResult` | `src/runtime/trigger_result.h` — `CONTINUE`, `FIRE`, `FIRE_AND_PURGE` |
| `EventTimeTrigger` | `src/runtime/triggers/event_time_trigger.h` — fires when watermark passes window end |
| `CountTrigger` | `src/runtime/triggers/count_trigger.h` — every N events |
| `WatermarkPropagator` | `src/runtime/watermark_propagator.h` — tracks min watermark across inputs |
| Wire triggers into window operators | Update TumblingWindow/SlidingWindow/Join to use triggers |

### Design Constraints

- Watermark is a special stream element that flows through the DAG like data
- `WatermarkPropagator` runs per operator instance:
  - Single-input: pass-through
  - Multi-input (join): take min of all input watermarks
- Watermark is broadcast to all instances of a downstream operator
- `TriggerResult::FIRE` = emit now, keep state; `FIRE_AND_PURGE` = emit and clear

### Tests

- Watermark flows through pipeline end-to-end
- Watermark cannot go backwards per input
- CountTrigger fires exactly every N events
- EventTimeTrigger fires when watermark ≥ window end
- Window + Trigger integration: events emitted when watermark triggers window close

---

## Phase 11: Placement Algorithms (Deferred)

**When**: Engine A exists and we have multiple workers/nodes.

**What it will do**:
- `PlacementStrategy` interface: `place(JobGraph, ClusterInfo) → PhysicalPlan`
- `GreedyPlacer` — first-fit, Flink-like
- `RoundRobinPlacer` — simple baseline for comparison
- `ILPPlacer` — uses ortools or similar for optimal placement
- `PlacementMetrics` — network cost, load balance, data locality scoring

---

## Phase 12: Complex Topology Validation

**Depends on**: Phase 10

### Deliverables

| Task | Files |
|------|-------|
| Integration tests | `tests/integration/` — end-to-end tests on non-trivial topologies |

### Topologies to validate

- **Linear pipeline**: source → map → filter → sink
- **Fan-out**: source → [map1, map2] → sink
- **Fan-in**: [source1, source2] → join → sink
- **Diamond**: source → [a, b] → join → sink
- **Multi-stage**: source → keyBy → window → reduce → sink
- **High parallelism**: operators with parallelism > 1

### Tests

- Each topology runs correctly on Pipeline (Phase 6), and later on Engine A/B
- Watermark propagates correctly through complex DAGs
- No deadlocks on fan-in with different input rates

---

## Deferred: Distributed Runtime

**When**: Single-node architecture is fully validated and we need multi-node execution.

**What it will do**:
- `NodeManager` — node discovery, registration, heartbeat
- `TaskDispatcher` — remote task launching across workers
- `RemoteChannel` — network-backed data channel (replaces in-process queue)
- `Serialization` — wire format for events, watermarks, control messages
- `JobClient` — CLI or library entrypoint for job submission
- `Protocol` — RPC definitions (gRPC or custom)

Workers can run different engine types as long as they implement the same `ExecutionEngine` interface.
