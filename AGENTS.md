# AGENTS.md — OpenCode Instructions

## Build & Toolchain

- **Language**: C++26
- **Compiler**: Clang 22
- **Build system**: CMake with Ninja generator
- **Linker**: mold (via `-fuse-ld=mold`)
- **Compiler cache**: ccache
- **Formatter**: clang-format (strict, run before commit)
- **Sanitizers**: address + undefined behavior (ASan + UBSan) enabled in dev builds
- **Tests**: strict unit tests required for all code

Build command (example):
```
cmake -B build -S .
ninja -C build
```

Run tests:
```
cd build && ctest --output-on-failure
```

Run formatting check:
```
find . -not -path './build/*' \( -name '*.cpp' -o -name '*.h' \) | xargs clang-format --dry-run --Werror
```

Auto-fix formatting:
```
ninja -C build format
```

## Project: Distributed Stream Computing Platform

### Design Philosophy

Extensibility is the primary goal, NOT extreme performance or feature richness.
The platform serves as a testbed for experimenting with and validating new stream computing algorithms.

### Programming Model

Dataflow programming model. Users define a DAG of operators connected by data streams.

### Key Architecture Decisions

#### Out-of-Order Processing
- Watermark mechanism with triggers for handling out-of-order events.
- Every operator respects watermarks; triggers fire when conditions are met.

#### Deferred Concerns (not yet designed)
- State management (checkpointing, state backends)
- Fault tolerance & high availability
- Load management / dynamic scaling

#### Execution Engines (Pluggable, Two Planned)

**Engine A — Fixed-Slot (Flink-like)**
Each worker has a fixed number of slots. Each task (operator instance) occupies one slot.
Static scheduling, predictable resource allocation.

**Engine B — Task-Stealing**
Workers share a task queue. Executors pull tasks from the queue and execute them.
- **Known problem**: Multiple executors consuming the same topic partition will produce out-of-order output. A mechanism to prevent this is planned but not yet designed.

Both engines must implement a common engine interface.

#### Operator Placement Algorithms (Pluggable)
Initial: greedy placement (Flink-like).
Future: ILP-based and other advanced placement algorithms.
All placement algorithms must implement a unified placement interface.

### Extensibility Points (Pluggable Interfaces)

1. **Execution engine** — slot-based vs. task-stealing vs. future engines
2. **Operator placement** — greedy, ILP, ML-based, etc.
3. These two are the current known extension points; the architecture should support adding more

### Code Structure Conventions

- Source: `src/` (engine core, operators, runtime)
- Headers: `include/` (public API)
- Tests: `tests/` (unit tests mirror src/ layout)
- Plugins/interfaces: `interfaces/` or a subdirectory thereof

### Commit & PR Workflow

- clang-format must pass before commit
- All unit tests must pass
- ASan+UBSan clean build required
- No commits of build artifacts or IDE files
- **After each session**: update [PLAN.md](./PLAN.md) to reflect completion status, then commit

### Code Style
- **No Implicit conversion**. For basic types like int, we have strong typed ones, use it instead.

- **No macros**. C++26 has replacements for every macro use case except platform conditionals:
  | Use case | Replacement |
  |----------|-------------|
  | Compile-time constants | `constexpr` |
  | Generics | `template` / `concept` |
  | Log location info | `std::source_location` |
  | Assertions | `static_assert` |
  | Compile-time code injection | CRTP / templates |
  | Header guards | `#pragma once` |
  | Platform conditionals | macro (no replacement — but project is Linux-only, so rarely needed) |
- **No raw pointers.** Use `const T&` for observation, `T`/`std::move` for ownership. A raw pointer cannot express "borrowed, don't delete me" — the compiler won't stop misuse. References encode this in the type system.
- **Keep it simple.** No over-engineering. Prefer straightforward solutions.
- Do NOT add comments unless asked.
- **Do NOT modify code unless explicitly asked.** When proposing changes, describe them first and wait for the user to say "go ahead".

### Development Workflow

Every coding task follows this order. Do NOT skip ahead.

1. **Discuss the approach first.** Talk through the design/options at a high level (what mechanism, which strategy, alternatives considered) until the user agrees on a direction. Do not touch files.
2. **Discuss implementation details before writing.** Show the concrete plan: which classes to add or change, their public interfaces, and their private members/state. Wait for the user to approve.
3. **Only then write code.** Still do not start until the user explicitly says to (e.g. "go ahead", "开始写", "先写这个").
4. **Write tests after the code.** Cover at minimum:
   - The simplest case that should pass
   - Cases that should fail / be rejected
   - Randomized or property-style tests where appropriate

Until the user explicitly authorizes writing, limit yourself to discussion, reading files, and showing planned interfaces.

### Key Technical Decisions

- **Testing framework**: GoogleTest (via CMake FetchContent)
- **Integer overflow**: Debug → assert/panic | Release → wrapping
- **File convention**: `.h` / `.cpp`, indent 4 spaces, column limit 100
- **Execution order**: single-node first, distributed runtime deferred to Phase 12
- **Platform**: Linux only

See [PLAN.md](./PLAN.md) for the full development roadmap.
