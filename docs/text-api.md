# Text API (`beez.text.*`)

String helpers for `build.lua`: search, transform, split/join, regex, and line diff.

No extra dependencies — implemented with the C++ standard library.

---

## Predicates

| Function | Description |
|----------|-------------|
| `contains(text, needle)` | `true` if `needle` appears in `text` |
| `starts_with(text, prefix)` | prefix check |
| `ends_with(text, suffix)` | suffix check |

---

## Case and replace

| Function | Description |
|----------|-------------|
| `to_lowercase(text)` | ASCII lower case |
| `to_uppercase(text)` | ASCII upper case |
| `to_case(text)` | title case (`"hello WORLD"` → `"Hello World"`) |
| `replace(text, search, replacement)` | first occurrence only |
| `replace_all(text, search, replacement)` | all occurrences |

`search` must not be empty for `replace` / `replace_all`.

---

## Split / join / trim

```lua
local parts = beez.text.split("a,b,c", ",")   -- { "a", "b", "c" }
local line = beez.text.join({ "a", "b" }, ", ") -- "a, b"
local trimmed = beez.text.trim("  spaced  ")
```

`split` requires a non-empty delimiter.

---

## Regex

```lua
beez.text.regex_match("abc123", "^abc%d+$")          -- true
beez.text.regex_replace("foo bar foo", "foo", "baz") -- "baz bar baz"
```

Uses ECMAScript syntax (`std::regex`). Invalid patterns throw.

---

## `beez.text.template(template_string, variables_table)`

**Walking skeleton** — returns `template_string` unchanged for now. A real template engine will be wired in later (e.g. for `--init` scaffolding).

---

## `beez.text.diff(old_text, new_text)`

Line-based diff (LCS). Returns a 1-based array of chunks:

```lua
{
    { op = "equal", text = "unchanged line" },
    { op = "insert", text = "new line" },
    { op = "delete", text = "removed line" },
}
```

Lines are split on `\n` (no trailing newline in `text` fields).

---

## Example

```lua
local version = beez.text.trim(beez.fs.read("VERSION"))
if not beez.text.regex_match(version, "^%d+%.%d+%.%d+$") then
    error("invalid VERSION: " .. version)
end

local tags = beez.text.split(version, ".")
local tagline = beez.text.join(tags, "-")
```

---

## See also

- [`data-api.md`](data-api.md) — `beez.data.diff` for **table** diffs (not strings)
- [`fs-api.md`](fs-api.md) — read/write files
