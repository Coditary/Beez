# Beez

Modern C++20 project with plugin architecture.

## Quick Start

```bash
# Install dependencies
make setup

# Build
make build

# Run
./build/bin/beez

# Test
make test
```

## Development

```bash
make help          # Show all targets
make format        # Format all code
make lint          # Run linters
make analyze       # Static analysis
make security      # Security checks
make all           # build + test + lint + analyze
```

## Build with Sanitizers

```bash
make sanitize      # ASan + UBSan debug build
```

## Requirements

- CMake >= 3.24
- Clang >= 17
- Conan 2
- ccache or sccache (optional)
