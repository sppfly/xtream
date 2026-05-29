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

## Phase 2: Core Data Model

**Depends on**: Phase 1

### Deliverables

| Task | Files |
|------|-------|
| `Event<T>` | `src/core/event.h` — payload + timestamp + metadata (opaque map) |
| `Watermark` | `src/core/watermark.h` — timestamp + source-id, comparison operators |
| `StreamSchema` | `src/core/stream_schema.h` — ordered list of (field-name, type-id) |
| `Record` | `src/core/record.h` — runtime-typed row: list of variant-like values + schema ref |

### Design Constraints

- `Event<T>` is the unit of data flow: every operator consumes/produces events
- `Watermark` is a special stream element (not a regular event) that flows through the DAG
- `Record` stores arbitrary typed fields for operators that don't know the schema at compile time
- Schemas are validated when edges are established (Phase 3)

### Tests

- Event copy/move, timestamp ordering
- Watermark comparison and merging (min of all inputs = aligned watermark)
- Schema matching / mismatch detection
- Record field access by name, type-safe getter

---

## Phase 3: Dataflow Graph (DAG)

**Depends on**: Phase 2

### Deliverables

| Task | Files |
|------|-------|
| `OperatorDescriptor` | `src/graph/operator_descriptor.h` — name, type, parallelism, input/output arity |
| `StreamEdge` | `src/graph/stream_edge.h` — source operator-id, target operator-id, partitioning (forward/keyed/broadcast) |
| `DataflowGraph` | `src/graph/dataflow_graph.h` — DAG builder, cycle detection, schema compatibility validation |

### Design Constraints

- Graph is immutable after construction (build once, validate, submit)
- Cycle detection uses DFS-based topological ordering
- Schema validation: output schema of upstream must match input schema of downstream for each edge
- Edge partitioning modes: `Forward` (same task), `Keyed` (hash-routed), `Broadcast` (all downstream)

### Tests

- Valid DAG passes validation
- Cycle rejection
- Schema mismatch rejection
- Edge arity mismatch rejection
- Multiple inputs to one operator (union), multiple outputs from one operator (fan-out)

---

## Phase 4: Operator Interface

**Depends on**: Phase 3

### Deliverables

| Task | Files |
|------|-------|
| `Operator` abstract class | `interfaces/operator.h` |
| `OneInputOperator` | `interfaces/one_input_operator.h` — for Map, Filter |
| `TwoInputOperator` | `interfaces/two_input_operator.h` — for Join |
| `SourceOperator` | `interfaces/source_operator.h` |
| `SinkOperator` | `interfaces/sink_operator.h` |
| `Collector` | `interfaces/collector.h` — output collector interface |
| `OperatorContext` | `interfaces/operator_context.h` — runtime config, metric counters, watermark access |
| `OperatorFactory` | `interfaces/operator_factory.h` — creates operator instances from descriptors |

### Operator Lifecycle

```
new → open() → [processElement() × N] → close() → delete
```

`SourceOperator`: `open()` → `run(Collector&)` → `close()`
`SinkOperator`: `open()` → `processElement(T)` → `close()`

### Design Constraints

- Operators are NOT thread-safe by default; the engine serializes access per instance
- `Collector.collect(Event)` is the only way to emit output
- `OperatorContext` provides access to config, metrics counters, and current watermark — read-only from operator's perspective

### Tests

- Lifecycle ordering (open → process → close)
- Collector delivers events in order
- OperatorContext returns correct config values
- Multiple operator instances from one factory

---

## Phase 5: Execution Engine Interface

**Depends on**: Phase 4

### Deliverables

| Task | Files |
|------|-------|
| `ExecutionEngine` | `interfaces/execution_engine.h` — `submit(JobGraph)`, `execute()`, `cancel()` |
| `JobGraph` | `interfaces/job_graph.h` — flattened task graph from DAG |
| `Task` | `interfaces/task.h` — single operator instance with assigned parallelism index |
| `TaskState` | `interfaces/task_state.h` — CREATED → SCHEDULED → RUNNING → FINISHED / FAILED / CANCELLED |
| `PhysicalPlan` | `interfaces/physical_plan.h` — result of placement: mapping Task → Worker |

### Design Constraints

- `ExecutionEngine` is the pluggable interface. All engine implementations inherit from it.
- `JobGraph` is produced by converting a `DataflowGraph` (Phase 3) into an executable plan.
- `Task` wraps one `Operator` instance + parallelism index (e.g., operator "map" with parallelism 4 produces 4 tasks).
- `PhysicalPlan` is the output of the placement algorithm.

### Tests

- JobGraph correctly expands parallel operators into N tasks
- Task state transitions valid (invalid transitions rejected)
- Engine interface can be mocked for testing

---

## Phase 6: Trigger & Watermark Framework

**Depends on**: Phase 4, Phase 2 (Watermark type)

### Deliverables

| Task | Files |
|------|-------|
| `Trigger` abstract class | `src/runtime/trigger.h` — `onEvent()`, `onWatermark()`, returns `TriggerResult` |
| `TriggerResult` | `src/runtime/trigger_result.h` — `CONTINUE`, `FIRE`, `FIRE_AND_PURGE` |
| `ProcessingTimeTrigger` | `src/runtime/triggers/processing_time_trigger.h` |
| `EventTimeTrigger` | `src/runtime/triggers/event_time_trigger.h` — fires when watermark passes window end |
| `CountTrigger` | `src/runtime/triggers/count_trigger.h` — every N events |
| `WatermarkPropagator` | `src/runtime/watermark_propagator.h` — tracks min watermark across inputs, emits aligned watermark |

### Design Constraints

- Triggers are used by window operators in Phase 9
- `WatermarkPropagator` runs per operator instance:
  - For single-input ops: pass-through
  - For multi-input ops (join): take min of all input watermarks
- Watermark is broadcast to all operator instances of a downstream operator
- `TriggerResult::FIRE` = emit window result now, keep state; `FIRE_AND_PURGE` = emit and clear

### Tests

- Watermark propagation: min of inputs is emitted downstream
- Watermark cannot go backwards per input
- CountTrigger fires exactly every N events
- EventTimeTrigger fires when watermark ≥ window end
- Multiple triggers can be composed

---

## Phase 7: Engine A — Fixed-Slot (Single-Node)

**Depends on**: Phase 5, Phase 6

### Deliverables

| Task | Files |
|------|-------|
| `SlotEngine` | `src/engine/slot/slot_engine.h` — implements `ExecutionEngine` |
| `SlotManager` | `src/engine/slot/slot_manager.h` — maintains N slots, assigns tasks to slots |
| `Slot` | `src/engine/slot/slot.h` — owns a thread, runs one task at a time |
| `GreedyPlacer` | `src/placement/greedy_placer.h` — first-fit operator placement |

### Architecture

```
SlotEngine
├── SlotManager (N slots)
│   ├── Slot[0] → TaskThread → Operator instance
│   ├── Slot[1] → TaskThread → Operator instance
│   └── ...
├── GreedyPlacer (produces PhysicalPlan)
└── WatermarkCoordinator (synchronizes watermarks across threads)
```

### Execution Flow

1. `submit(JobGraph)` → `GreedyPlacer.place()` → `PhysicalPlan`
2. Instantiate operators per plan
3. Wire collectors between tasks according to edge partitioning
4. `execute()` → start all slots → run until all sources exhausted and all watermarks sentinel
5. Graceful shutdown: close sinks first, then upstream

### Design Constraints

- One task per slot, one thread per slot
- Inter-task communication via in-process queues (e.g., `moodycamel::ConcurrentQueue` or a ring buffer)
- Watermark broadcast: watermark coordinator collects watermarks from all tasks and broadcasts min
- Slots are static; no dynamic resizing in Phase 7

### Tests

- Simple pipeline (source → map → sink) runs end-to-end
- Pipeline with parallelism > 1 produces correct results
- Watermark flows through pipeline
- Two-branch DAG (fan-out then fan-in)
- Graceful shutdown completes all in-flight events

---

## Phase 8: Basic Operators

**Depends on**: Phase 7

### Deliverables

| Task | Files |
|------|-------|
| `CollectionSource` | `src/operators/collection_source.h` — emits from in-memory vector (for testing) |
| `MapOperator` | `src/operators/map_operator.h` — transforms Event<IN> → Event<OUT> |
| `FlatMapOperator` | `src/operators/flatmap_operator.h` — produces 0..N output events per input |
| `FilterOperator` | `src/operators/filter_operator.h` — drops events not matching predicate |
| `CollectSink` | `src/operators/collect_sink.h` — appends to in-memory vector (for testing) |
| `PrintSink` | `src/operators/print_sink.h` — prints events to stdout |

### Design Constraints

- Operators are templated on input/output types where compile-time typing is possible
- For runtime-typed data (Record), type checks happen at graph validation time (Phase 3)
- `MapFunction`, `FilterFunction`, `FlatMapFunction` are function objects / lambdas passed to operators

### Tests

- Map transforms every event
- Filter correctly drops non-matching events
- FlatMap produces 0..N outputs per input
- Source emits all elements, Sink collects all
- End-to-end: CollectionSource → Map → Filter → CollectSink

---

## Phase 9: Advanced Operators

**Depends on**: Phase 7, Phase 6

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
- Window operators hold state (deferred to later), but in Phase 9 keep it in-memory with triggers from Phase 6
- Windows fire when the watermark passes `window_end + allowed_lateness`
- Join uses `WatermarkPropagator` from Phase 6 to align watermarks from both sides

### Tests

- Tumbling window aggregates events into correct windows
- Sliding window with slide < size produces overlapping windows
- Reduce computes correct cumulative result
- Join pairs events with matching keys within time window
- Late events (watermark already passed) are dropped or side-output

---

## Phase 10: Engine B — Task-Stealing (Single-Node)

**Depends on**: Phase 7

### Deliverables

| Task | Files |
|------|-------|
| `StealingEngine` | `src/engine/stealing/stealing_engine.h` — implements `ExecutionEngine` |
| `TaskQueue` | `src/engine/stealing/task_queue.h` — thread-safe multi-producer multi-consumer queue |
| `Executor` | `src/engine/stealing/executor.h` — worker thread that pulls and runs tasks |
| `PartitionGuard` | `src/engine/stealing/partition_guard.h` — mechanism to prevent concurrent execution of same partition |

### Architecture

```
StealingEngine
├── Shared TaskQueue (priority queue, tasks ordered by watermark age)
├── Executor threads × M
│   └── each pulls from queue, executes one event, pushes back if not drained
└── PartitionGuard: ensures only one executor processes a given partition at a time
```

### Problem: Out-of-Order Output

Multiple executors consuming the same topic partition produce out-of-order output. Solution: `PartitionGuard`.
Each partition (source shard / key group) has a lock or token. An executor must acquire the token before processing that partition's next event. This serializes per-partition processing while allowing parallelism across partitions.

### Design Constraints

- Same interface as `SlotEngine` (Phase 7) — drop-in replacement from the DAG / Operator perspective
- Task queue uses work-stealing: if an executor's local queue is empty, it steals from another
- Event ordering within a partition is preserved; ordering across partitions is not guaranteed (by design)
- Graceful shutdown: poison-pill sentinel in queue

### Tests

- Same end-to-end tests as Phase 8 pass with StealingEngine
- Same-partition events are processed in order
- Events from different partitions can be processed concurrently
- PartitionGuard prevents concurrent access to same partition
- Work-stealing: idle executor picks up work from busy executor

---

## Phase 11: Advanced Placement Algorithms

**Depends on**: Phase 7, Phase 10

### Deliverables

| Task | Files |
|------|-------|
| `PlacementStrategy` interface | `interfaces/placement_strategy.h` — `place(JobGraph, ClusterInfo) → PhysicalPlan` |
| `GreedyPlacer` | `src/placement/greedy_placer.h` — refactored to implement `PlacementStrategy` |
| `RoundRobinPlacer` | `src/placement/round_robin_placer.h` — simple baseline |
| `ILPPlacer` | `src/placement/ilp_placer.h` — integer linear programming (use `ortools` or similar) |
| `PlacementMetrics` | `src/placement/metrics.h` — network cost, load balance score, data locality |

### Design Constraints

- `PlacementStrategy` is the unified interface — all placement algorithms implement it
- `ClusterInfo` describes available workers: slot count, memory, network topology
- Placement result is validated: every task assigned, no slot overbooked
- ILP solver should be an optional dependency (e.g., CMake option `ENABLE_ILP`)
- Benchmark: compare greedy vs ILP on random DAGs (network cost, load balance)

### Tests

- GreedyPlacer assigns all tasks without overbooking
- RoundRobinPlacer distributes evenly
- ILPPlacer produces feasible solution (all constraints met)
- PlacementStrategy can be swapped without changing engine code
- Metrics correctly compute cost scores

---

## Phase 12: Distributed Runtime

**Depends on**: Phase 7, Phase 10, Phase 11

### Deliverables

| Task | Files |
|------|-------|
| `NodeManager` | `src/runtime/node_manager.h` — node registration, heartbeat, discovery |
| `TaskDispatcher` | `src/runtime/task_dispatcher.h` — remote task launching |
| `RemoteChannel` | `src/runtime/remote_channel.h` — network-backed data channel (replaces in-process queue) |
| `Serialization` | `src/runtime/serialization.h` — wire format for events, watermarks, control messages |
| `JobClient` | `src/client/job_client.h` — CLI or library entrypoint for job submission |
| `Protocol` | `src/runtime/protocol.h` — RPC definitions (gRPC or custom) |

### Design Constraints

- In-process channels (Phase 7) and remote channels share the same abstract `Channel` interface
- Serialization is pluggable (protobuf default, flatbuffers optional)
- Heartbeat: workers report to master every N seconds; master marks dead workers after timeout
- Network failures: initially just fail the job; HA is deferred

### Architecture

```
                 ┌────────────────┐
                 │   JobClient    │
                 └───────┬────────┘
                         │ submit(JobGraph)
                 ┌───────▼────────┐
                 │    Master      │
                 │  ├─NodeManager │
                 │  ├─Placer      │
                 │  └─Dispatcher  │
                 └───────┬────────┘
           ┌─────────────┼─────────────┐
    ┌──────▼──────┐ ┌────▼──────┐ ┌────▼──────┐
    │   Worker 1  │ │ Worker 2  │ │ Worker 3  │
    │ (SlotEngine)│ │(StealingE)│ │(SlotEngine)│
    │ ┌──┐ ┌──┐   │ │ TaskQueue │ │ ┌──┐ ┌──┐  │
    │ │S1│ │S2│   │ │ JobQueue  │ │ │S1│ │S2│  │
    │ └──┘ └──┘   │ └──────────┘ │ └──┘ └──┘  │
    └─────────────┘ └────────────┘ └────────────┘
```

Note: workers can run different engine types as long as they implement `ExecutionEngine`.

### Tests

- Two-node pipeline (source → map → sink) runs end-to-end
- Events serialized, transmitted, and deserialized correctly
- Node heartbeat / failure detection
- Task launched on correct remote worker per PhysicalPlan
- Different engine types can coexist in same cluster
```

