# Vertical Feature Development in Beez

This document describes how to implement a feature **end-to-end** (vertically), from user story to completed quality assurance.

When you describe a new feature, the full workflow below should always be followed, not just a single layer.

---

## What does "vertical" mean?

A feature is implemented **across all affected layers**:

```
User Story
    ↓
Tests first (unit → integration → system)   ← TDD
    ↓
Core (models, registry, orchestrator)
    ↓
Plugins (Lua DSL, shell executor)
    ↓
CLI (main.cpp)
    ↓
Refactor + quality assurance (make all)
```

**Not** vertical: Only extending `Registry` with no DSL binding, no tests, no CLI behavior.

**Vertical**: The user story is usable, tested, and verified by all QA steps.

---

## Project structure (quick reference)

| Area | Path | Purpose |
|------|------|---------|
| Headers | `include/beez/` | Public API for all modules |
| Core | `src/core/` | Models, registry, orchestrator, plugin host |
| Lua plugin | `src/plugins/lua/` | DSL parsing (`build.lua`) |
| Shell plugin | `src/plugins/shell/` | Command execution |
| App | `src/app/` | CLI entry point |
| Unit tests | `tests/unit/` | Mirrors `src/` structure |
| Integration tests | `tests/integration/` | Interaction between components |
| System tests | `tests/system/` | Black-box with fixture projects |
| Fuzzer | `tests/fuzz/` | Parser robustness (Lua DSL) |
| QA reports | `report/` | Generated output from `make test`, `lint`, `analyze`, etc. |

Modules are **flat** under `src/`, no nested `src/` or `include/` per module.

---

## Step-by-step: new feature

### 1. Clarify the user story

Before writing code, define:

- **What** should the user be able to do?
- **Acceptance criteria**: When is the feature done?
- **Which layers** are affected?

Example user story:

> As a user, I want to define tasks with a `depends_on` field so that dependencies between tasks can be modeled.

Affected layers: `Task` model, `Registry`, Lua DSL, possibly `Orchestrator`, unit + integration + system tests.

---

### 2. TDD: Red → Green → Refactor

Use **Test-Driven Development** for every slice of the feature. Write tests **before** production code, then implement the minimum to pass, then refactor.

#### Red, write a failing test

1. Pick the smallest testable behavior from the acceptance criteria
2. Write the test at the appropriate level (unit first, then integration, then system)
3. Register new test files in `CMakeLists.txt`
4. Run tests and confirm they **fail** (compile errors or assertion failures are expected)

#### Green, make the test pass

1. Implement the **minimum** code needed to pass the test
2. Work layer by layer: core → plugins → orchestrator/CLI
3. Run tests again and confirm they **pass**

#### Refactor, improve without changing behavior

1. Clean up duplication, naming, and structure
2. Re-run tests after each refactor to ensure they still pass
3. Fix lint/format issues (`make format`, `make lint`)

Repeat Red → Green → Refactor for each acceptance criterion until the feature is complete.

**Example cycle for `depends_on`:**

```
Red:   test that a task with depends_on is parsed correctly → fails (not implemented)
Green: add depends_on field to Task + Lua DSL parser → test passes
Refactor: extract shared parsing helper, run tests again

Red:   test that orchestrator runs dependencies in order → fails
Green: implement dependency ordering in orchestrator → test passes
Refactor: simplify, run tests again
```

---

### 3. Tests (required, all applicable levels)

Write tests **first** (see TDD above), at every level that applies:

| Level | When | Where | What to test |
|-------|------|-------|--------------|
| **Unit** | Whenever core/plugin logic changes | `tests/unit/` (same structure as `src/`) | Isolated functions, positive + negative cases |
| **Integration** | When components interact | `tests/integration/` | Pipeline with real plugins, CLI as subprocess |
| **System** | When end-to-end behavior is user-visible | `tests/system/scenarios/` + `fixtures/` | Black-box with `build.lua` fixtures |

**Test rules:**

- **Tests before implementation**, never add production code without a failing test first
- Register new `.cpp` files in the respective `CMakeLists.txt`
- Unit tests mirror the `src/` folder structure (`tests/unit/core/`, `tests/unit/plugins/lua/`, …)
- System tests: fixture project under `tests/system/fixtures/<name>/` with `build.lua`
- Cover positive **and** negative cases (invalid inputs, missing fields, unknown tasks)

---

### 4. Implementation (inside out)

Only write production code to make failing tests pass (Green phase).

#### 4a. Core models and logic

1. Add or extend types/structs in `include/beez/core/`
2. Implement logic in `src/core/`
3. Update `CMakeLists.txt` in `src/core/` for new `.cpp` files

#### 4b. Plugins

- **Lua DSL** (`src/plugins/lua/lua_dsl.cpp`): Parse new DSL syntax and map to core models
- **Shell executor** (`src/plugins/shell/`): Only when command execution changes

#### 4c. Orchestrator / plugin host

- Extend execution logic in `src/core/orchestrator.cpp` when the feature affects runtime behavior

#### 4d. CLI

- Touch `src/app/main.cpp` only when CLI behavior or error messages change

---

### 5. Parser / DSL changes: fuzzer

When `lua_dsl.cpp` or DSL syntax changes:

1. Add a seed file at `tests/fuzz/corpus/lua_dsl/<descriptive_name>.lua`
2. Only commit `.lua` files with descriptive names, **no** hash artifacts
3. `make fuzzer-smoke` must pass

---

### 6. Quality assurance (required before "done")

At the end, **always** run the full pipeline:

```bash
make all
```

This includes:

| Step | Target | Checks |
|------|--------|--------|
| Build | `build` | Compiles release |
| Tests | `test` | Unit + integration + system |
| Format | `format-check` | clang-format + cmake-format |
| Lint | `lint` | clang-tidy |
| Static analysis | `analyze` | cppcheck + clang-tidy |
| Security | `security` | Snyk / security scripts |
| Coverage | `coverage` | Code coverage |
| Sanitizer | `sanitize` | ASan + UBSan |
| Fuzzer | `fuzzer-smoke` | Lua DSL robustness |

**Definition of done:** `make all` completes without errors.

On failure: fix issues and rerun `make all`, do not mark as done prematurely.

---

## Per-feature checklist

Copy this checklist into the feature description or PR:

```
[ ] User story and acceptance criteria are clear
[ ] Failing tests written first (Red)
[ ] Minimum implementation to pass tests (Green)
[ ] Code refactored, tests still pass (Refactor)
[ ] Core models/logic implemented
[ ] Plugins updated (if DSL/execution affected)
[ ] Orchestrator/CLI updated (if needed)
[ ] CMakeLists.txt updated (new files)
[ ] Unit tests (positive + negative)
[ ] Integration tests (if components interact)
[ ] System tests + fixtures (if end-to-end behavior)
[ ] Fuzzer seed (if DSL changed)
[ ] make all, green
```

---

## Example: "task with phase field" feature

**User story:** Tasks can be assigned to a phase.

| TDD step | File | Change |
|----------|------|--------|
| Red | `tests/unit/plugins/lua/test_lua_dsl.cpp` | Test parsing with/without `phase` → fails |
| Green | `include/beez/core/task.hpp` | Add `std::optional<std::string> phase` |
| Green | `src/plugins/lua/lua_dsl.cpp` | Parse `phase` field from Lua table → unit test passes |
| Red | `tests/integration/plugins/test_lua_shell_pipeline.cpp` | Test phase task registration → fails |
| Green | (same DSL changes) | Integration test passes |
| Red | `tests/system/fixtures/phase-tasks/build.lua` | System scenario → fails |
| Green | Wire through orchestrator if needed | System test passes |
| Refactor | All touched files | Clean up, `make all` green |

---

## Prompt template for new features

When describing a feature to Cursor/AI, this is enough:

```
Implement feature: <short description>

User story:
As a <role>, I want <action> so that <benefit>.

Acceptance criteria:
- ...
- ...

Please implement vertically with TDD (Red → Green → Refactor)
per docs/vertical-feature.md (tests first, then make all at the end).
```

The Cursor rule `.cursor/rules/vertical-feature.mdc` ensures QA steps are included automatically.

---

## What not to do

- Write production code before tests (no TDD)
- Implement a feature in only one layer and leave tests for "later"
- Commit hash files from the fuzzer into `tests/fuzz/corpus/`
- Skip `make all` or only run `make test`
- Forget to add new files to `CMakeLists.txt`
- Create nested module folders (`src/core/src/`)
