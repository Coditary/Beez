## Summary

<!-- What changed and why? Link related issues (e.g. Fixes #123). -->

## Type of change

- [ ] Bug fix
- [ ] New feature
- [ ] Breaking change
- [ ] Documentation (wiki or in-repo)
- [ ] Build / CI / tooling
- [ ] Refactor (no behavior change)

## How was this tested?

<!-- New or updated tests, manual steps, `beez` commands you ran. -->

- [ ] `make test`
- [ ] `make all` (full local QA pipeline)
- [ ] Other (describe below)

## User-visible changes

<!-- CLI flags, DSL, config keys, default behavior. "None" if internal only. -->

## Breaking changes

<!-- List breaking changes or write "None". -->

## Documentation

- [ ] Wiki updated (required for user-visible CLI, DSL, config, cache, or UI changes)
- [ ] `CHANGELOG.md` updated (if appropriate)
- [ ] In-repo docs updated (`README.md`, `docs/`, as needed)
- [ ] Not needed (internal-only change)

## Checklist

- [ ] New source and test files are registered in CMake where needed
- [ ] DSL or parser changes include fuzz corpus updates when appropriate (`tests/fuzz/corpus/lua_dsl/`)
- [ ] Line coverage on `src/` ≥ 85% (`make coverage`)
- [ ] No unrelated drive-by refactors mixed into this change
- [ ] I have read [`CONTRIBUTING.md`](CONTRIBUTING.md)
