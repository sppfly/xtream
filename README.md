# xtream

A distributed stream computing platform written in C++26.

## What This Is

xtream is a dataflow-based stream processing engine. Users define a DAG of operators connected by data streams, and the platform executes them with support for out-of-order event processing via watermarks and triggers.

This is not a production system. It is a testbed for experimenting with and validating new stream computing algorithms — particularly around execution engines, operator placement, and watermark propagation.

## Design Philosophy

**Extensibility over performance.** The primary goal is a pluggable architecture where execution engines, placement algorithms, and other components can be swapped without modifying core code.

**Single-node first.** Each engine must be fully functional on a single node before going distributed. This simplifies development and testing.

**Every phase delivers testable code.** No phase produces only documentation or stubs. All code is covered by unit tests and built with ASan+UBSan.

## Current State

Phase 6 complete. The platform can:

- Define operator DAGs with a fluent builder API
- Compile logical operators to physical execution chains
- Execute linear pipelines single-threaded (Source -> Map -> Filter -> Sink)
- Handle source functions (callable that produces events) and sink functions (callable that processes events)

### Example

```cpp
auto graph = DataflowGraphBuilder()
    .source([]() -> Event<Record> { return generate_event(); })
    .map([](Event<Record>& e) -> Event<Record> { return transform(e); })
    .filter([](Event<Record>& e) -> bool { return e.timestamp() > cutoff; })
    .sink([](Event<Record>& e) { process(e); })
    .build();

Pipeline pipeline(std::move(graph));
pipeline.run(1000);
```

## Architecture

```
interfaces/          Pluggable API contracts (function concepts, operator interfaces)
src/
  core/              Fundamental types (Event, Record, Watermark, strict integers)
  engine/            Compiler, Pipeline
  graph/             DataflowGraph, DataflowGraphBuilder (fluent API)
  operators/         Physical operator implementations
  util/              Logger
tests/               Unit tests (mirrors src/ and interfaces/ layout)
```

## Build

Requires: Clang 22, Ninja, mold linker, ccache.

```bash
cmake -B build -S .
ninja -C build
cd build && ctest --output-on-failure
```

Formatting:
```bash
ninja -C build format
```

## Roadmap

| Phase | Status | Description |
|-------|--------|-------------|
| 1 | Done | Infrastructure: strict types, logger, project skeleton |
| 2 | Done | Core data model: Event, Watermark, StreamSchema, Record |
| 3 | Done | Dataflow graph: DAG representation, cycle detection |
| 4 | Done | Operator interfaces + simple operators (Map, Filter, Source, Sink) |
| 5 | Done | Compilation bridge: DataflowGraph -> PhysicalOperator chain |
| 6 | Done | Pipeline: single-threaded execution with lifecycle management |
| 7 | Pending | Engine A: fixed-slot execution (Flink-like) |
| 8 | Pending | Engine B: task-stealing execution |
| 9 | Pending | Advanced operators: KeyBy, TumblingWindow, SlidingWindow, Reduce, Join |
| 10 | Pending | Watermark propagation + trigger framework |
| 11 | Pending | Placement algorithms (greedy, ILP) |
| 12 | Pending | Complex topology validation (integration tests) |

See [PLAN.md](./PLAN.md) for detailed phase specifications.

## Code Style

- C++26, no macros, no raw pointers, no implicit conversions
- `#pragma once`, 4-space indent, 100-column limit
- clang-format enforced, all tests must pass before commit
- Linux only
