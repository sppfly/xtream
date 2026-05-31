---
name: dev-phase
description: Standard workflow for completing a development phase in the Extream project — compile, test, format, and commit.
---

# Phase Completion Workflow

After completing a phase or sub-phase of PLAN.md, execute these steps in order.

## 1. Compile

```bash
cmake -B build -S .
ninja -C build
```

## 2. Run Tests

```bash
cd build && ctest --output-on-failure
```

All tests must pass before moving to the next step.

## 3. Format In-Place

Run clang-format to format all source files directly. Do NOT run `--dry-run` to check — just format:

```bash
ninja -C build format
```

## 4. Commit

```bash
git add -A
git commit -m "<phase or sub-phase description>"
```

Do not commit build artifacts or IDE files (ensured by `.gitignore`).
