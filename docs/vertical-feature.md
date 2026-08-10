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
CLI (main.cpp / cli when user-visible)
    ↓
Documentation (wiki, CHANGELOG, in-repo docs)
    ↓
Refactor + quality assurance (make all, coverage ≥ 85%)
```

**Not** vertical: Only extending `Registry` with no DSL binding, no tests, no CLI behavior.

**Vertical**: The user story is usable, tested, and verified by all QA steps.

---

## Project structure (quick reference)

| Area | Path | Purpose |
|------|------|---------|
| Headers | `include/beez/` | Public API for all modules |
| Core | `src/core/` | Domain modules (model, registry, config, cache, …) — single `beez_core` library |
| Lua plugin | `src/plugins/lua/` | DSL parsing (`build.lua`) |
| Shell plugin | `src/plugins/shell/` | Command execution |
| App | `src/app/` | CLI entry point |
| Unit tests | `tests/unit/` | Mirrors `src/` structure |
| Integration tests | `tests/integration/` | Interaction between components |
| System tests | `tests/system/` | Black-box with fixture projects |
| Fuzzer | `tests/fuzz/` | Parser robustness (Lua DSL) |
| QA reports | `report/` | Generated output from `make test`, `lint`, `analyze`, etc. |

Modules are **flat** under `src/` (no per-module CMake targets). Within `src/core/`, code is grouped by domain folder; `include/beez/core/` mirrors the same layout. Public headers use `.hpp`.

### Core layout (`include/beez/core/` ↔ `src/core/`)

| Domain | Headers | Sources | Tests (`tests/unit/core/`) |
|--------|---------|---------|----------------------------|
| `model/` | task, step, workflow, step_config, … | `model/` | `test_workflow.cpp` |
| `util/` | expected, text_table, temp_directory | `util/` | `test_text_table.cpp`, `test_temp_directory.cpp` |
| `registry/` | registry, step_order | `registry/` | `registry/` |
| `config/` | `settings/`, `cache/`, `performance/`, `env/`, `ui/`, `schema/`, `paths/`, `report/` | `config/` | `config/{settings,cache,performance,env,ui,schema,paths,report}/` |
| `cache/` | `storage/`, `fingerprint/`, `step/`, `success/` | `cache/` | `cache/{storage,fingerprint,step,success}/` |
| `glob/` | pattern, expand, metadata_cache | `glob/` | `glob/` |
| `env/` | env_file | `env/` | `env/` |
| `runtime/` | context | `runtime/` | `runtime/` |
| `execution/` | `concurrency/` (thread_pool, worker_pool), `process/` (stream_capture) | `execution/` | `execution/{concurrency,process}/` |
| `orchestrator/` | `orchestrator`, `errors`, `types`, `run/` (`stats`, `lifecycle`), `runners/` | `orchestrator/` | `orchestrator/` |

Plugin host lives under `include/beez/plugin/` / `src/plugins/` (not in `core/`).

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

#### 4c. Orchestrator

- Extend execution logic in `src/core/orchestrator/runners/` when the feature affects runtime behavior (e.g. `step.cpp`, `phase.cpp`)

#### 4d. CLI

- Touch `src/app/main.cpp` only when CLI behavior or error messages change

---

### 5. Documentation (required for user-visible changes)

Documentation is part of a vertical feature, not a follow-up task.

#### Wiki ([Beez Wiki](https://github.com/Coditary/Beez/wiki))

The wiki is a separate git repository on GitHub. Update it when behavior visible to users changes:

| Area | Typical pages |
|------|----------------|
| CLI flags or commands | CLI Flag Reference, CLI Overview, Quick Reference |
| Config keys / merge order | Config Reference, Configuration Overview |
| DSL / `build.lua` | DSL Overview, Step/Task/Workflow declaration pages |
| Caching | Caching chapter, troubleshooting if behavior changed |
| Terminal UI | UI and Output chapter (modes, themes, summaries, logs) |
| Contributor workflow | Feature Development Workflow, Code Quality, Building and Setup |

Push wiki changes to the wiki repo so they are live before or right after the code merge.

#### In-repository docs

| File | When to update |
|------|----------------|
| [`CHANGELOG.md`](CHANGELOG.md) | User-visible changes, fixes, breaking changes |
| [`README.md`](README.md) | Install paths, quick start, high-level features |
| [`CONTRIBUTING.md`](CONTRIBUTING.md) | PR process, CI, quality targets |
| [`docs/`](README.md) | Developer workflow, architecture notes |
| Issue/PR templates | Process or checklist changes |

See [`docs/README.md`](README.md) for the full documentation map.

**Definition of done for docs:** wiki and in-repo docs match the behavior on `main`. No stale flag tables, config keys, or examples.

---

### 6. Parser / DSL changes: fuzzer

When `lua_dsl.cpp` or DSL syntax changes:

1. Add a seed file at `tests/fuzz/corpus/lua_dsl/<descriptive_name>.lua`
2. Only commit `.lua` files with descriptive names, **no** hash artifacts
3. `make fuzzer-smoke` must pass

---

### 7. Quality assurance (required before "done")

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
| Coverage | `coverage` | Line coverage on `src/` ≥ **85%** (enforced by `scripts/coverage-report.sh`) |
| Sanitizer | `sanitize` | ASan + UBSan |
| ThreadSanitizer | `tsan` | Data races / thread safety |
| Fuzzer | `fuzzer-smoke` | Lua DSL robustness |

Check coverage locally:

```bash
make coverage
# Open report/coverage/index.html
```

Override the threshold for local experiments only: `MIN_LINE_COVERAGE=80 make coverage`.

**Definition of done:** `make all` completes without errors and line coverage on `src/` is at least 85%. CI runs the same checks in parallel (see [`CONTRIBUTING.md`](../CONTRIBUTING.md)).

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
[ ] Wiki updated (if user-visible behavior changed)
[ ] CHANGELOG.md / README / docs updated (as appropriate)
[ ] make coverage ≥ 85% line coverage on src/
[ ] make all green
```

---

## Example: "task with phase field" feature

**User story:** Tasks can be assigned to a phase.

| TDD step | File | Change |
|----------|------|--------|
| Red | `tests/unit/plugins/lua/test_lua_dsl.cpp` | Test parsing with/without `phase` → fails |
| Green | `include/beez/core/model/task.hpp` | Add `std::optional<std::string> phase` |
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
per docs/vertical-feature.md (tests first, update wiki/docs for user-visible
changes, make all + coverage ≥ 85% at the end).
```

See also [`docs/README.md`](README.md) for where user vs contributor documentation lives.

---

## What not to do

- Write production code before tests (no TDD)
- Implement a feature in only one layer and leave tests for "later"
- Commit hash files from the fuzzer into `tests/fuzz/corpus/`
- Skip `make all` or only run `make test`
- Ship user-visible features without updating the wiki or CHANGELOG
- Merge when line coverage on `src/` is below 85%
- Forget to add new files to `CMakeLists.txt`
- Create nested module folders (`src/core/src/`)
