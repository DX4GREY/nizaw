# Development

## Repository layout

```text
CMakeLists.txt
README.md
include/nizaw/          # public headers
include/nizaw/core/     # core module public headers (error, log, platform, env, version)
src/                    # implementation files
src/cli/                # CLI entry point and dispatch
src/core/               # core module implementation
src/system/             # system module implementation
src/process/            # process module implementation
src/filesystem/         # filesystem module implementation
src/storage/            # storage module implementation
src/network/            # network module implementation
src/service/            # service module implementation
src/security/           # security module implementation
src/plugin/             # plugin module implementation
examples/               # example programs
tests/                  # unit and integration tests
docs/                   # design and wiki documentation
```

## Build locally

```bash
cmake -S . -B build -G Ninja -DNIZAW_BUILD_CLI=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Build examples

```bash
cmake -S . -B build -G Ninja -DNIZAW_BUILD_EXAMPLES=ON
cmake --build build --parallel --target nizaw_example
./build/examples/nizaw_example
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
- Use `NIZAW_LOG_*` macros for diagnostics instead of raw `std::cerr` in library code
- Use `nizaw::Error::from_errno` when wrapping syscall failures

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
| Core error handling | Implemented | `nizaw::Error`, `nizaw::ErrorCode`, `Result<T>` |
| Logging | Implemented | `nizaw::core::Logger` with level filtering |
| Platform detection | Implemented | `/etc/os-release` parsing, systemd detection |
| Environment helpers | Implemented | `nizaw::core::env::get`, `get_or`, `exists` |
| Version metadata | Implemented | `nizaw::core::version()` — v3.0.0 |
| System info | Implemented | Hostname, kernel, uptime, CPU, memory, swap, load, modules, hwmon |
| Process inspection | Implemented | Process list, inspect, environment, signals, terminate, suspend, resume, nice |
| Filesystem operations | Implemented | Usage, info, mounts, mkdir, rm, cp, mv, chmod, chown, write, read, truncate |
| Storage information | Implemented | Device listing, inspect, I/O stats |
| Network info | Implemented | Interface listing, inspect, connections |
| Service management | Implemented | systemd list, inspect, start, stop, restart, reload, enable, disable |
| Security context | Implemented | UIDs, GIDs, groups, capabilities |
| Plugin loading | Implemented | Dynamic discovery and loading of `.so` files with API versioning |
| Remote transport | Implemented | mTLS transport, certificate fingerprinting, HTTP/2 client |
| Agent orchestration | Implemented | Agent lifecycle, config, telemetry, task executor, task queue |

## Contributing

Contributions are welcome. Keep changes focused, add tests for behavioral changes, and update the documentation when public behavior or build steps change.