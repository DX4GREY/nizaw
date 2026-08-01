# Architecture

## Design goals

Nizaw is organized around a simple rule: the CLI must not contain logic that the library does not also expose. Library modules own the system-facing logic; the CLI only parses arguments, calls the library, and formats output.

## High-level structure

```text
nizaw
├── core
│   ├── error
│   ├── log
│   ├── platform
│   ├── env
│   └── version
├── system
├── process
├── filesystem
├── storage
├── network
├── service
├── security
├── plugin
└── cli
```

## Module overview

| Module | Responsibility | Why it exists |
| --- | --- | --- |
| `core` | Error handling (`nizaw::Error`, `nizaw::ErrorCode`), logging (`nizaw::core::Logger`), platform detection (`nizaw::core::detect`), environment helpers (`nizaw::core::env`), version metadata (`nizaw::core::version`) | Provides the shared base layer for the rest of the framework |
| `system` | Hostname, kernel info, uptime, boot time, CPU count, page size | Lets applications ask basic host questions in a structured way |
| `process` | Process enumeration and inspection via `/proc` | Useful for debugging, monitoring, and process inventory |
| `filesystem` | Disk usage and filesystem metadata | Useful for diagnostics and storage visibility |
| `storage` | Block device enumeration and metadata | Gives visibility into disks, block devices, and storage topology |
| `network` | Interface enumeration and address reporting | Helps inspect networking state without relying on shell wrappers |
| `service` | Service listing and status introspection | Makes service state visible in a consistent format |
| `security` | UID/GID/group and capability reporting | Useful for privilege and security context inspection |
| `plugin` | Discovering and loading plugin modules | Enables extension without modifying the core build |
| `cli` | Command parsing, formatting, and exit code behavior | Gives the project a user-friendly interface on top of the library |

## How it works

1. The library modules talk directly to Linux interfaces such as `/proc`, `/sys`, `/dev`, `ioctl`, and `getifaddrs`.
2. Each fallible public API returns `nizaw::Result<T>` with structured error information.
3. The CLI consumes these APIs and prints either human-readable text or JSON.
4. Plugins are discovered from a directory and loaded dynamically as shared objects.

## Error handling

Expected failures such as missing files, permission errors, or vanished processes are represented with `nizaw::Result<T>` instead of exceptions. This keeps the public API predictable and script-friendly.

`nizaw::Error` carries a stable `ErrorCode` enum, a human-readable message, the source module, and an optional raw `errno` value. `Result<void>` is provided for fallible operations with no meaningful success value.

## Logging

The `nizaw::core::Logger` is a minimal, dependency-free logging sink with levels `Trace`, `Debug`, `Info`, `Warn`, `Error`, `Fatal`, and `Off`. It writes to stderr (stdout is reserved for command output/JSON) and is guarded by a mutex for thread safety. Convenience macros `NIZAW_LOG_TRACE`, `NIZAW_LOG_DEBUG`, `NIZAW_LOG_INFO`, `NIZAW_LOG_WARN`, `NIZAW_LOG_ERROR`, and `NIZAW_LOG_FATAL` are provided.

## Platform detection

`nizaw::core::detect()` reads `/etc/os-release` to report the distro id, name, and version, and detects systemd via the presence of `/run/systemd/system`. It never fails outright — unknown distro state is represented with empty fields rather than an error.

## Dependency policy

The default posture is zero required third-party runtime dependencies. The project prefers the C++20 standard library and direct Linux APIs over external packages where possible. The only optional dependency is `libsystemd`/`sd-bus` for the `service` module; when unavailable, a stub implementation is built instead.

## Extension points

- Add a new module by introducing a new target in `CMakeLists.txt`
- Expose the new API through headers under `include/nizaw`
- Implement the logic in `src/<module>/`
- Add tests under `tests/<module>/`
- Wire the command into the CLI if it should be user-facing