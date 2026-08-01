# Development

## Repository layout

```text
CMakeLists.txt
README.md
include/nizaw/        # public headers
src/                  # implementation files
src/cli/              # CLI entry point and dispatch
tests/                # unit and integration tests
```

## Build locally

```bash
cmake -S . -B build -G Ninja -DNIZAW_BUILD_CLI=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Add a new module

1. Create a new header in `include/nizaw/`
2. Implement the module in `src/<module>/`
3. Register the target in `CMakeLists.txt`
4. Add tests under `tests/<module>/`
5. Link the module from the CLI or other consumers as needed

## Coding conventions

- Prefer `nizaw::Result<T>` for fallible public APIs
- Keep public headers free from platform-specific implementation details
- Avoid shelling out to external commands when a Linux API exists
- Keep the CLI thin; put logic in the library layer
- Favor clear module boundaries over cross-module shortcuts

## Testing

The test suite is built by default. You can run specific checks with:

```bash
ctest --test-dir build --output-on-failure -V
```

## Recommended workflow

```bash
cmake -S . -B build -DNIZAW_BUILD_CLI=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/nizaw --help
```

## Why this project exists

This project exists to make Linux system introspection easier to build and reuse from C++. Instead of manually parsing `/proc`, `/sys`, or systemd-related state from scratch, developers can rely on a small, structured API and a consistent CLI.

## Feature matrix

| Capability | Status | Notes |
| --- | --- | --- |
| System info | Implemented | Hostname, kernel, uptime, CPU, architecture |
| Process inspection | Implemented | Process list and detailed inspect support |
| Filesystem info | Implemented | Usage and metadata reporting |
| Storage information | Implemented | Device listing and metadata |
| Network info | Implemented | Interface and address reporting |
| Service info | Implemented | Service listing/status visibility |
| Security context | Implemented | UIDs, GIDs, groups, capabilities |
| Plugin loading | Implemented | Dynamic discovery and loading of `.so` files |

## Contributing

Contributions are welcome. Keep changes focused, add tests for behavioral changes, and update the documentation when public behavior or build steps change.
