# Network API (`beez.net.*`)

HTTP downloads and REST helpers for `build.lua`, backed by **libcurl**.

Paths passed to `download`, `download_and_verify`, and `upload` are resolved relative to the project root (like `beez.fs`).

---

## Headers

Pass headers as a **string-keyed Lua table** (header name → header value):

```lua
local headers = {
    ["Content-Type"] = "application/json",
    Accept = "application/json",
    Authorization = "Bearer " .. token,
}

local response = beez.net.get("https://api.example.com/v1/status", headers)
```

For `request` and `download`, headers live inside an options table:

```lua
beez.net.request("PUT", url, {
    headers = headers,
    body = payload,
})
```

Header names are sent as provided; response headers are normalized to **lowercase** keys in the result table.

---

## REST

All REST helpers return a table:

| Field | Type | Description |
|-------|------|-------------|
| `status` | number | HTTP status code |
| `body` | string | Response body |
| `ok` | boolean | `true` when status is 2xx |
| `headers` | table | Response headers (`name` → `value`) |
| `error` | string | Present only on transport failure (curl error) |

Transport failures throw a Lua error. HTTP 4xx/5xx return normally with `ok = false`.

```lua
local response = beez.net.get(url, headers)
local response = beez.net.post(url, body, headers)
local response = beez.net.put(url, body, headers)
local response = beez.net.delete(url, headers)

local response = beez.net.request(method, url, {
    body = "...",
    headers = headers,
    timeout = 30,              -- seconds (default 30)
    follow_redirects = true,     -- default true
})
```

`upload(url, file_path, [headers])` sends a **multipart/form-data** POST with field name `file`.

---

## Downloads

```lua
local result = beez.net.download(url, "artifacts/tool.tar.gz", {
    headers = headers,
    timeout = 120,
    follow_redirects = true,
})
-- result.path, result.bytes

beez.net.download_and_verify(url, dest, "sha256", expected_hex_hash, {
    headers = headers,
})
-- result.path, result.bytes, result.hash, result.verified
```

`download_and_verify` throws on hash mismatch.

---

## Connectivity

```lua
if beez.net.is_online() then ... end          -- optional timeout seconds: is_online(5)

local ping = beez.net.ping("https://example.com", 10)
-- ping.ok, ping.status, ping.ms, ping.error?
```

`is_online()` probes a small set of public endpoints with a short timeout.

---

## Proxy

```lua
beez.net.set_proxy("http://127.0.0.1:8080")
beez.net.set_proxy(nil)   -- clear
```

Applies to subsequent requests in the same process.

---

## Performance notes

- libcurl with **keep-alive**, **compressed encoding** (`Accept-Encoding`), and HTTP/2 when available
- One easy handle per request (simple and thread-safe with Beez's worker model)

---

## See also

- [`worker-api.md`](worker-api.md) — in-step workers
- [`CHANGELOG.md`](../CHANGELOG.md)
