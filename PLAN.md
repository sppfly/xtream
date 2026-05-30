# Development Plan — Extream Stream Computing Platform

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

## Phase 4: Operator Interface + Simple Operators

**Depends on**: Phase 3

### Deliverables

| Task | Files |
|------|-------|
| `Operator` abstract class | `interfaces/operator.h` |
| `OneInputOperator` | `interfaces/one_input_operator.h` |
| `TwoInputOperator` | `interfaces/two_input_operator.h` |
| `SourceOperator` | `interfaces/source_operator.h` |
| `SinkOperator` | `interfaces/sink_operator.h` |
| `Collector` | `interfaces/collector.h` — output collector interface |
| `OperatorContext` | `interfaces/operator_context.h` — runtime config, metric counters, watermark access |
| `CollectionSource` | `src/operators/collection_source.h` — emits from in-memory vector |
| `MapOperator` | `src/operators/map_operator.h` — transforms Event<IN> → Event<OUT> |
| `FilterOperator` | `src/operators/filter_operator.h` — drops events not matching predicate |
| `CollectSink` | `src/operators/collect_sink.h` — appends to in-memory vector |

### Design Constraints

- `Collector.collect(Event)` is the only way to emit output
- Operators are NOT thread-safe by default; the engine serializes access per instance
- `OperatorContext` provides access to config, metrics counters, and current watermark
- `MapFunction`, `FilterFunction` are function objects / lambdas passed to operators
- Lifecycle: `open()` → `[processElement() × N]` → `close()`

### Tests

- Lifecycle ordering (open → process → close)
- Collector delivers events in order
- Map transforms every event
- Filter correctly drops non-matching events
- End-to-end: CollectionSource → Map → Filter → CollectSink

---

## Phase 5: Engine A — Fixed-Slot (Single-Node)

**Depends on**: Phase 4

### Deliverables

| Task | Files |
|------|-------|
| `ExecutionEngine` interface | `interfaces/execution_engine.h` — `submit(JobGraph)`, `execute()`, `cancel()` |
| `JobGraph` | `interfaces/job_graph.h` — flattened task graph from DAG |
| `Task` | `interfaces/task.h` — single operator instance with assigned parallelism index |
| `TaskState` | `interfaces/task_state.h` — CREATED → SCHEDULED → RUNNING → FINISHED / FAILED / CANCELLED |
| `PhysicalPlan` | `interfaces/physical_plan.h` — mapping Task → Worker |
| `SlotEngine` | `src/engine/slot/slot_engine.h` — implements `ExecutionEngine` |
| `SlotManager` | `src/engine/slot/slot_manager.h` — maintains N slots, assigns tasks to slots |
| `Slot` | `src/engine/slot/slot.h` — owns a thread, runs one task at a time |
| `GreedyPlacer` | `src/placement/greedy_placer.h` — first-fit operator placement |

### Execution Flow

1. `submit(JobGraph)` → `GreedyPlacer.place()` → `PhysicalPlan`
2. Instantiate operators per plan
3. Wire collectors between tasks according to edge partitioning
4. `execute()` → start all slots → run until all sources exhausted
5. Graceful shutdown: close sinks first, then upstream

### Design Constraints

- `ExecutionEngine` is the pluggable interface — both engines inherit from it
- One task per slot, one thread per slot
- Inter-task communication via in-process queue (thread-safe MPSC or ring buffer)
- Slots are static; no dynamic resizing

### Tests

- Simple pipeline (source → map → sink) runs end-to-end
- Pipeline with parallelism > 1 produces correct results
- Two-branch DAG (fan-out then fan-in)
- Graceful shutdown completes all in-flight events

---

## Phase 6: Watermark Propagation + Trigger Framework

**Depends on**: Phase 4, Phase 5

### Deliverables

| Task | Files |
|------|-------|
| `Trigger` abstract class | `src/runtime/trigger.h` — `onEvent()`, `onWatermark()`, returns `TriggerResult` |
| `TriggerResult` | `src/runtime/trigger_result.h` — `CONTINUE`, `FIRE`, `FIRE_AND_PURGE` |
| `EventTimeTrigger` | `src/runtime/triggers/event_time_trigger.h` — fires when watermark passes window end |
| `CountTrigger` | `src/runtime/triggers/count_trigger.h` — every N events |
| `WatermarkPropagator` | `src/runtime/watermark_propagator.h` — tracks min watermark across inputs |

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

---

## Phase 7: Engine B — Task-Stealing (Single-Node)

**Depends on**: Phase 5

### Deliverables

| Task | Files |
|------|-------|
| `StealingEngine` | `src/engine/stealing/stealing_engine.h` — implements `ExecutionEngine` |
| `TaskQueue` | `src/engine/stealing/task_queue.h` — thread-safe multi-producer multi-consumer queue |
| `Executor` | `src/engine/stealing/executor.h` — worker thread that pulls and runs tasks |
| `PartitionGuard` | `src/engine/stealing/partition_guard.h` — prevents concurrent execution of same partition |

### Problem: Out-of-Order Output

Multiple executors consuming the same topic partition produce out-of-order output. Solution: `PartitionGuard`.
Each partition has a lock/token. An executor must acquire it before processing that partition's next event. This serializes per-partition processing while allowing parallelism across partitions.

### Design Constraints

- Same interface as SlotEngine — drop-in replacement
- Task queue uses work-stealing: idle executor steals from busy one
- Event ordering within a partition is preserved; across partitions is not guaranteed
- Graceful shutdown: poison-pill sentinel in queue

### Tests

- Same end-to-end tests pass with StealingEngine
- Same-partition events are processed in order
- PartitionGuard prevents concurrent access to same partition
- Work-stealing: idle executor picks up work from busy executor

---

## Phase 8: Advanced Operators

**Depends on**: Phase 6

### Deliverables

| Task | Files |
|------|-------|
| `KeyByOperator` | `src/operators/keyby_operator.h` — routes events to downstream by key (mod N) |
| `TumblingWindow` | `src/operators/tumbling_window.h` — fixed-size non-overlapping windows |
| `SlidingWindow` | `src/operators/sliding_window.h` — overlapping windows |
| `WindowAssigner` | `src/operators/window_assigner.h` — assigns events to window(s) |
| `ReduceOperator` | `src/operators/reduce_operator.h` — incremental aggregation |
| `WindowApply` | `src/operators/window_apply.h` — applies function to windowed data on trigger |
| `JoinOperator` | `src/operators/join_operator.h` — inner join on key, time-windowed |

### Design Constraints

- `KeyBy` changes edge partitioning to `Keyed` — affects how the engine routes events
- Window operators hold state in-memory; triggers from Phase 6
- Windows fire when the watermark passes `window_end + allowed_lateness`
- Join uses `WatermarkPropagator` to align watermarks from both sides

### Tests

- Tumbling window aggregates events into correct windows
- Sliding window with slide < size produces overlapping windows
- Join pairs events with matching keys within time window
- Late events (watermark already passed) are dropped

---

## Phase 9: Placement Algorithms

**Depends on**: Phase 5, Phase 7

### Deliverables

| Task | Files |
|------|-------|
| `PlacementStrategy` interface | `interfaces/placement_strategy.h` — `place(JobGraph, ClusterInfo) → PhysicalPlan` |
| `GreedyPlacer` | `src/placement/greedy_placer.h` — refactored to implement `PlacementStrategy` |
| `RoundRobinPlacer` | `src/placement/round_robin_placer.h` — simple baseline |
| `ILPPlacer` | `src/placement/ilp_placer.h` — integer linear programming (use `ortools` or similar) |
| `PlacementMetrics` | `src/placement/metrics.h` — network cost, load balance score, data locality |

### Design Constraints

- `PlacementStrategy` is the unified interface — all algorithms implement it
- `ClusterInfo` describes available workers: slot count, memory, network topology
- Placement result is validated: every task assigned, no slot overbooked
- ILP solver is an optional dependency (CMake option `ENABLE_ILP`)

### Tests

- GreedyPlacer assigns all tasks without overbooking
- RoundRobinPlacer distributes evenly
- ILPPlacer produces feasible solution
- PlacementStrategy can be swapped without changing engine code

---

## Phase 10: Complex Topology Validation

**Depends on**: Phase 8

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

- Each topology runs correctly on both Engine A and Engine B
- Correct output ordering within partitions
- Watermark propagates correctly through complex DAGs
- No deadlocks on fan-in with different input rates

---

## Deferred: Distributed Runtime

Not part of current plan. Will be designed after single-node architecture is fully validated.

