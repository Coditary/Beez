# Worker API (`ctx:spawn`, `ctx:wait`, `ctx:wait_all`)

Workers let a Lua step callback run shell commands in the background (from Beez's perspective they are queued and executed when you `wait`). Use them to parallelize work inside a single step or to integrate with the step cache via `inputs` / `outputs`.

Only available inside `step({ run = function(ctx) ... end })`.

---

## Quick example

```lua
step({
    name = "compile",
    phase = "compile",
    scope = "cpp",
    run = function(ctx)
        local jobs = {}

        for _, source in ipairs(ctx.glob({ "src/**/*.cpp" })) do
            jobs[#jobs + 1] = ctx:spawn({
                cmd = "g++ -c " .. source .. " -o build/" .. source:gsub("src/", ""):gsub("%.cpp$", ".o"),
                inputs = { source },
                outputs = { "build/" .. source:gsub("src/", ""):gsub("%.cpp$", ".o") },
            })
        end

        local results = ctx:wait_all(jobs, { exitCode = true, output = true })
        for _, result in ipairs(results) do
            if result.exitCode ~= 0 then
                error("compile failed:\n" .. (result.output or ""))
            end
        end

        return 0
    end,
})
```

Workers spawned in this step are named automatically: `compile-1`, `compile-2`, …

---

## `ctx:spawn(options)`

Registers a worker. Returns an integer handle (worker id).

### Options

| Field | Required | Type | Description |
|-------|----------|------|-------------|
| `cmd` | yes | `string` or `{ string, ... }` | Shell command(s) to run in order; stops on first non-zero exit |
| `inputs` | no | `{ string, ... }` | Glob patterns for step-cache input tracking |
| `outputs` | no | `{ string, ... }` | Glob patterns for step-cache output tracking |
| `name` | no | `string` | Override auto-generated name (rare; logging/UI only) |

`name` is **not** required. Beez assigns `{step-name}-{n}` using the enclosing step's name and a per-step counter.

### Example

```lua
local job = ctx:spawn({
    cmd = "make -j8",
    outputs = { "build/**" },
})
```

Multiple commands in one worker:

```lua
local job = ctx:spawn({
    cmd = {
        "mkdir -p build",
        "cmake --build build",
    },
})
```

---

## `ctx:wait(handle [, options])`

Waits until the worker finishes (or returns immediately if already done).

### Return value

| Call | Returns |
|------|---------|
| `ctx:wait(job)` | `nil` — wait only |
| `ctx:wait(job, {})` | `nil` — empty options table |
| `ctx:wait(job, { exitCode = true, ... })` | Table with only the requested fields |

### Options (all boolean, default `false`)

| Key | Type | Description |
|-----|------|-------------|
| `exitCode` | `number` | Process exit code (last command if multiple) |
| `output` | `string` | Captured stdout+stderr (merged) |
| `duration` | `number` | Seconds — execution time, or saved time on cache hit |
| `cached` | `boolean` | `true` if step cache skipped execution |
| `name` | `string` | Worker name (e.g. `compile-1`) |
| `id` | `number` | Handle id |
| `dryRun` | `boolean` | Whether the run was a dry-run |

Unknown option keys raise an error.

### Examples

```lua
-- Block only
ctx:wait(job)

-- Exit code
local result = ctx:wait(job, { exitCode = true })
if result.exitCode ~= 0 then
    return result.exitCode
end

-- Failure details
local fail = ctx:wait(job, { exitCode = true, output = true })
if fail.exitCode ~= 0 then
    print(fail.output)
end
```

Calling `wait` again on the same handle is idempotent — results come from stored state.

`wait` with `exitCode == 0` also records worker duration for `ctx.cache_file_success(path)` (same as before).

---

## `ctx:wait_all([handles] [, options])`

Waits for multiple workers. The same `options` table applies to **all** workers (one table, no per-job overhead).

### Forms

| Call | Behavior | Returns |
|------|----------|---------|
| `ctx:wait_all()` | Drain all spawned workers in this step | `nil` |
| `ctx:wait_all(jobs)` | Wait for listed handles (in parallel when thread pool allows) | `nil` |
| `ctx:wait_all(jobs, { exitCode = true })` | Wait, then return result tables | `{ result, ... }` (1-based array) |
| `ctx:wait_all(nil, { exitCode = true })` | Drain all, return all results in spawn order | Array of tables |

Result order matches the `jobs` table order (or worker id order when draining all).

### Example

```lua
local jobs = { job_a, job_b }
local results = ctx:wait_all(jobs, { exitCode = true, duration = true })

for index, result in ipairs(results) do
    print(jobs[index], result.exitCode, result.duration, result.cached)
end
```

---

## Step cache

Workers with `inputs` and/or `outputs` participate in the **step cache** (same as before). Cache identity uses the parent step name and command/artifacts — not the `compile-N` display suffix — so identical workers can still cache-hit across runs.

On cache hit:

- `exitCode` is `0`
- `cached` is `true`
- `output` is `""`
- `duration` reflects saved time from the cache entry

---

## Auto-drain

If the step callback returns without calling `wait` / `wait_all`, Beez drains remaining workers automatically before the step finishes (unchanged).

---

## Migration from older behavior

| Before | After |
|--------|-------|
| `local code = ctx:wait(job)` | `local r = ctx:wait(job, { exitCode = true }); local code = r.exitCode` |
| `return ctx:wait(job)` | `return ctx:wait(job, { exitCode = true }).exitCode` |
| `return ctx:wait_all(jobs)` | `ctx:wait_all(jobs); return 0` or inspect `ctx:wait_all(jobs, { exitCode = true })` |
| `ctx:spawn({ name = "x", cmd = ... })` | `ctx:spawn({ cmd = ... })` — name optional |

---

## See also

- [Step cache / caching](https://github.com/Coditary/Beez/wiki/Caching) (wiki)
- [`docs/test-feature-matrix.md`](test-feature-matrix.md) — test coverage map
- [`CHANGELOG.md`](../CHANGELOG.md) — release notes
