#!/usr/bin/env bash
# Regenerate field-matrix fuzz seeds from DSL unit-test cases.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="${ROOT}/tests/fuzz/corpus/lua_dsl"

write_seed() {
  local name="$1"
  local body="$2"
  printf '%s\n' "${body}" >"${OUT}/field_${name}.lua"
}

write_seed "step_only_name" 'step({ name = "s" })'
write_seed "step_missing_run" 'step({ name = "s", phase = "p", scope = "sc" })'
write_seed "step_invalid_phase_type" 'step({ name = "s", phase = 42, scope = "sc", run = "true" })'
write_seed "step_optional_description" 'step({ name = "s", phase = "p", scope = "sc", description = "docs", run = "true" })'
write_seed "step_empty_artifacts" 'step({ name = "s", phase = "p", scope = "sc", input = {}, output = {}, mutate = {}, run = "true" })'

write_seed "task_empty_action_table" 'task("empty", {})'
write_seed "task_invalid_action_type" 'task("broken", { 42 })'
write_seed "task_phase_shorthand_table" 'task("broken", { phase = "p", scope = "sc", run = "true" })'
write_seed "task_step_ref_missing_name" 'task("broken", { { config = { x = 1 } } })'
write_seed "task_step_ref_with_config" 'step({ name = "s", phase = "p", scope = "sc", run = "true" })
task("run", { { name = "s", config = { flag = true } } })'

write_seed "workflow_valid_phase_scope" 'step({ name = "s", phase = "p", scope = "sc", run = "true" })
workflow("run", { { phase = "p", scope = "sc" } })'
write_seed "workflow_parallel_rejected" 'workflow("run", { { parallel = { { phase = "p", scope = "sc" } } } })'
write_seed "workflow_empty_parallel_rejected" 'workflow("run", { { parallel = {} } })'
write_seed "workflow_rejects_numeric" 'workflow("run", { 42 })'

write_seed "config_empty_table" 'beez.config({})
task("hello", "true")'
write_seed "config_invalid_performance_type" 'beez.config({ performance = "not-a-table" })
task("hello", "true")'
write_seed "config_invalid_cache_path_type" 'beez.config({ cache = { path = 42 } })
task("hello", "true")'
write_seed "config_valid_cache_path" 'beez.config({ cache = { path = ".cache-custom" } })
task("hello", "true")'

write_seed "configure_step_before_registration" 'configure_step("future", { flag = true })
step({ name = "future", phase = "p", scope = "sc", run = "true" })'
write_seed "order_unknown_steps" 'order("missing-a", "missing-b")
task("hello", "true")'

write_seed "fixture_cache_adversarial" "$(cat "${ROOT}/tests/system/fixtures/cache-adversarial/build.lua")"
write_seed "fixture_workflows" "$(cat "${ROOT}/tests/system/fixtures/workflows/build.lua")"
write_seed "fixture_phase_tasks" "$(cat "${ROOT}/tests/system/fixtures/phase-tasks/build.lua")"
write_seed "fixture_config_env" "$(cat "${ROOT}/tests/system/fixtures/config-env/build.lua")"
write_seed "fixture_config_ui" "$(cat "${ROOT}/tests/system/fixtures/config-ui/build.lua")"
write_seed "fixture_config_cache" "$(cat "${ROOT}/tests/system/fixtures/config-cache/build.lua")"
write_seed "fixture_negative_invalid_config" "$(cat "${ROOT}/tests/system/fixtures/negative-invalid-config/build.lua")"
write_seed "fixture_negative_invalid_task_action" "$(cat "${ROOT}/tests/system/fixtures/negative-invalid-task-action/build.lua")"
write_seed "fixture_negative_empty_task" "$(cat "${ROOT}/tests/system/fixtures/negative-empty-task/build.lua")"
write_seed "fixture_negative_empty_workflow" "$(cat "${ROOT}/tests/system/fixtures/negative-empty-workflow/build.lua")"
write_seed "fixture_negative_step_missing_run" "$(cat "${ROOT}/tests/system/fixtures/negative-step-missing-run/build.lua")"
write_seed "fixture_negative_invalid_step_run" "$(cat "${ROOT}/tests/system/fixtures/negative-invalid-step-run/build.lua")"
write_seed "fixture_dsl_partial_step" "$(cat "${ROOT}/tests/system/fixtures/dsl-partial-step/build.lua")"
write_seed "fixture_dsl_workflow_no_steps" "$(cat "${ROOT}/tests/system/fixtures/dsl-workflow-no-steps/build.lua")"
write_seed "fixture_dsl_task_missing_step" "$(cat "${ROOT}/tests/system/fixtures/dsl-task-missing-step/build.lua")"

echo "Wrote field_* seeds to ${OUT}"
