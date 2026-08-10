# Archive API (`beez.archive.*`)

Create and read archives from `build.lua`, backed by **libarchive**.

Paths are resolved relative to the project root (like `beez.fs`).

Supported formats:

| Format | Extensions / `format` option |
|--------|------------------------------|
| ZIP | `.zip`, `zip` |
| tar | `.tar`, `tar` |
| gzip-compressed tar | `.tar.gz`, `.tgz`, `tar.gz`, `tgz` |
| bzip2-compressed tar | `.tar.bz2`, `.tbz2`, `tar.bz2`, `tbz2` |
| xz-compressed tar | `.tar.xz`, `.txz`, `tar.xz`, `txz` |

`compress` infers the format from the archive file name unless `format` is set explicitly.

---

## `beez.archive.compress(src_path, archive_path, [options])`

Compress a file or directory.

```lua
beez.archive.compress("dist", "release.zip")
beez.archive.compress("payload.txt", "payload.tar.gz", { format = "tar.gz" })
```

Returns `{ path = "...", bytes = N }`.

---

## `beez.archive.extract(archive_path, dest_dir)`

Extract all entries into `dest_dir` (created if needed). Path traversal (`..`, absolute paths) is blocked.

Returns `{ entries = N, dest = "..." }`.

---

## `beez.archive.list(archive_path)`

Returns a 1-based array of entry tables:

```lua
{
    { path = "src/main.cpp", size = 128, is_dir = false },
    { path = "assets", size = 0, is_dir = true },
}
```

---

## `beez.archive.extract_file(archive_path, file_in_archive, dest_path)`

Extract a single file.

Returns `{ path = "...", bytes = N }`.

---

## `beez.archive.read_text(archive_path, file_in_archive)`

Read a single text file from an archive (max 16 MiB). Throws if the entry is missing, is a directory, or is too large.

---

## Example

```lua
beez.archive.compress("vendor/src", "vendor.tar.gz")

local manifest = beez.archive.read_text("vendor.tar.gz", "vendor/src/VERSION")
print("vendor version:", manifest)

beez.archive.extract("vendor.tar.gz", "vendor unpacked")
```

---

## See also

- [`net-api.md`](net-api.md) — downloads
- [`CHANGELOG.md`](../CHANGELOG.md)
