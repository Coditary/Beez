# Beez Plugin Publishing

Beez workflow plugins are distributed as ReqPack `.rqp` packages and installed through the bundled `beez` ReqPack package manager.

## Plugin registry

The plugin catalog lives in a separate repository:

- Local checkout: `~/Dev/Coditary/beez-registry`
- GitHub: [Coditary/beez-registry](https://github.com/Coditary/beez-registry)
- Catalog file: `plugins.json`

The `beez` PM fetches `plugins.json` from that repo. Each entry points at the Beez release asset `plugins-index.json`, which lists platform-specific `.rqp` downloads.

Refresh the catalog after adding or changing plugins under `plugins/coditary/`:

```bash
lua scripts/ci/sync_plugin_catalog.lua
```

Commit and push the changes in `beez-registry` separately from Beez releases.

## Clang plugin monorepo (`beez-clang`)

Clang plugins (`clang`, `clang-format`, `clang-tidy`) live in a shared repository:

- Local checkout: `~/Dev/Coditary/beez-plugins/beez-clang`
- GitHub: [Coditary/beez-clang](https://github.com/Coditary/beez-clang)

Bootstrap or refresh from the Beez monorepo:

```bash
lua scripts/ci/bootstrap_beez_clang_monorepo.lua --init-git
```

Each plugin is a subdirectory with its own `beez.package.json`. The catalog references them via git source + path:

```json
"source": {
  "type": "git",
  "url": "github:Coditary/beez-clang",
  "path": "clang-format",
  "ref": "1.0.0"
}
```

In `build.lua` you can also use:

```lua
source = "github:Coditary/beez-clang#clang-format"
```

## Standalone plugin repositories

Official plugins can live in separate Git repositories under `~/Dev/Coditary/beez-plugins/`.
Bootstrap or refresh them from the monorepo:

```bash
lua scripts/ci/bootstrap_standalone_plugins.lua --init-git
```

Each directory is a complete Beez plugin (`beez.package.json`, `beez_plugin.lua`, `src/`, `README.md`, `LICENSE`).
Suggested GitHub naming: `Coditary/beez-plugin-<name>`.

## Monorepo plugins

The `beez` PM can install from a subdirectory of a git repository:

- `source = "github:Coditary/beez-clang#clang-format"` in `build.lua`
- catalog field `source.path` with `source.type = "git"`

For release-based distribution, attach per-plugin indexes under the monorepo release, e.g.
`https://github.com/Coditary/beez-clang/releases/download/v1.0.0/clang-format/index.json`.

## Package layout

```
plugins/<organization>/<name>/<version>/
  beez_plugin.lua
  src/
  scripts/
```

## Build a plugin package

```bash
lua scripts/ci/package_beez_plugin.lua \
  --organization coditary \
  --name clang-format \
  --version 1.0.0 \
  --source-dir plugins/coditary/clang-format/1.0.0 \
  --platform linux \
  --arch x86_64 \
  --output-dir dist/plugins
```

Build an index for release assets:

```bash
lua scripts/ci/build_plugin_index.lua \
  --dist-dir dist/plugins \
  --output dist/plugins/index.json
```

## Install plugins

After installing the Beez CLI (`rqp install rqp:beez-cli`), the `beez` package manager is available separately:

```bash
# PM auto-downloads from 0_registry on first use if missing
rqp install beez coditary/clang-format@1.0.0
```

Registry split (same model as Tempify):

| Name | Role | Install |
|------|------|---------|
| `beez-cli` | CLI `.rqp` package | `rqp install rqp:beez-cli` |
| `beez` | Plugin package manager | `rqp install beez org/plugin@version` |

Installing `beez-cli` also bundles the PM for offline use; `rqp install beez …` can still fetch the PM from registry when it is not installed yet.

## Project declaration (`build.lua`)

```lua
reqpack {
  beez = {
    { name = "coditary/clang-format", path = "./plugins/coditary/clang-format", version = "1.0.0" },
    { name = "coditary/clang-tidy", version = "1.0.0" },
    { name = "acme/custom-linter", version = "1.0.0", source = "github:acme/beez-custom-linter" },
  },
}
```

- `path` — local development plugin
- `version` only — install from registry catalog / plugin index
- `source` — git/GitHub fallback when the plugin is not in the catalog

## Git repository convention

For plugins distributed outside the catalog:

```
my-beez-plugin/
  beez.package.json
  beez_plugin.lua
  src/
```

`beez.package.json`:

```json
{
  "schemaVersion": 1,
  "organization": "acme",
  "name": "custom-linter",
  "version": "1.0.0",
  "entry": "beez_plugin.lua"
}
```
