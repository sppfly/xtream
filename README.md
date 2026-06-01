# xtream

A distributed stream computing platform written in C++26.

## What This Is

xtream is a dataflow-based stream processing engine. Users define a DAG of operators connected by data streams, and the platform executes them with support for out-of-order event processing via watermarks and triggers.

This is not a production system. It is a testbed for experimenting with and validating new stream computing algorithms — particularly around execution engines, operator placement, and watermark propagation.

## Design Philosophy

**Extensibility over performance.** The primary goal is a pluggable architecture where execution engines, placement algorithms, and other components can be swapped without modifying core code.

**Single-node first.** Each engine must be fully functional on a single node before going distributed. This simplifies development and testing.

**Every phase delivers testable code.** No phase produces only documentation or stubs. All code is covered by unit tests and built with ASan+UBSan.


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


## Code Style

- C++26, no macros, no raw pointers, no implicit conversions
- `#pragma once`, 4-space indent, 100-column limit
- clang-format enforced, all tests must pass before commit
- Linux only
