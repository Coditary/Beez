# Security Policy

## Supported versions

Beez is pre-1.0. Security fixes are applied on the latest `main` branch. Tagged releases, when published, receive fixes for the current minor release.

| Version | Supported |
|---------|-----------|
| `main` (latest) | Yes |
| Older tags / branches | Best effort |

Check your version with:

```bash
beez --version
```

## Reporting a vulnerability

**Please do not report security vulnerabilities in public GitHub issues.**

Use one of these channels:

1. **[GitHub Security Advisories](https://github.com/Coditary/Beez/security/advisories/new)** (preferred)
2. Contact the maintainers privately if you cannot use GitHub Advisories

We will acknowledge valid reports and work on a fix. For severe issues we aim to coordinate disclosure after a patch is available.

### What to include

- Description of the issue and impact
- Steps to reproduce, or a minimal PoC if possible
- Beez version or commit hash (`beez --version`, `git rev-parse HEAD`)
- OS, compiler, and environment details
- Whether the issue involves untrusted `build.lua`, shell commands, cache files, or config

### What we consider in scope

Examples of security-relevant reports:

- Memory corruption or unsafe execution in the Beez binary
- Command injection or path traversal when running steps or loading project files
- Cache or log handling that allows privilege escalation or arbitrary file access
- Secrets exposed in logs or cache fingerprints despite `env.mask_secrets`

Out of scope (use regular [issues](https://github.com/Coditary/Beez/issues) instead):

- Bugs with no security impact
- Hardening ideas without a demonstrated vulnerability
- Vulnerabilities in third-party dependencies already tracked upstream (please link the upstream advisory)
- Misconfiguration of a user's own `build.lua` or shell commands (users define what Beez runs)

## Security practices in this repository

- **CI:** `make security` runs clang-tidy security checks and cppcheck security rules (see `scripts/security.sh`)
- **CodeQL:** C++ analysis on pushes to `main` and `develop`, on PRs to `main`, and weekly (`.github/workflows/codeql.yml`)
- **Sanitizers:** ASan/UBSan test runs in CI (`make sanitize`)
- **Fuzzer:** Lua DSL fuzz smoke tests in CI (`make fuzzer-smoke`)

Contributors should run `make all` before opening pull requests; see [`CONTRIBUTING.md`](CONTRIBUTING.md).

## Safe use

Beez executes shell commands and Lua callbacks defined in project `build.lua` files. Treat project roots and build scripts like source code you trust:

- Only run Beez in repositories you trust
- Review `build.lua`, `config.lua`, and `.env` before running in sensitive environments
- Use `env.mask_secrets` and `env.hash_vars` appropriately; see the [wiki](https://github.com/Coditary/Beez/wiki/Environment-Variables)

## License

Security reports and coordinated fixes are handled under the same [Apache-2.0](LICENSE) license as the project.
