# Text API (`beez.text.*`)

String helpers for `build.lua`: search, transform, split/join, regex, templating, and line diff.

Most helpers use the C++ standard library. `beez.text.template` uses [Prebyte](https://github.com/Coditary/Prebyte), loaded on first use.

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

Renders a Prebyte template string with variables from a Lua table.

```lua
beez.text.template("Hello {{ name }}", { name = "Beez" })  -- "Hello Beez"

beez.text.template("{{ if ready }}yes{{ else }}no{{ endif }}", { ready = true })  -- "yes"

beez.text.template("{{ items[1] }}-{{ user.name }}", {
    items = { "a", "b" },
    user = { name = "Ada" },
})  -- "a-Ada"
```

Uses Prebyte `{{ ... }}` syntax: interpolation, conditionals, loops, filters, includes, and more. See the [Prebyte README](https://github.com/Coditary/Prebyte) for full template syntax.

The Prebyte engine is initialized lazily on the first `template()` call.

Invalid templates throw with a `beez.text.template:` prefix.

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
