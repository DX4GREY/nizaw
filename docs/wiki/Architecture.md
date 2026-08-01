# Architecture

## Design goals

Nizaw is organized around a simple rule: the CLI must not contain logic that the library does not also expose. Library modules own the system-facing logic; the CLI only parses arguments, calls the library, and formats output.

## High-level structure

```text
nizaw
├── core
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

| Module | Responsibility |
| --- | --- |
| `core` | Error handling, logging, version metadata, platform helpers, environment utilities |
| `system` | Hostname, kernel info, uptime, boot time, CPU count, page size |
| `process` | Process enumeration and inspection via `/proc` |
| `filesystem` | Disk usage and filesystem metadata |
| `storage` | Block device enumeration and metadata |
| `network` | Interface enumeration and address reporting |
| `service` | Service listing and status introspection |
| `security` | UID/GID/group and capability reporting |
| `plugin` | Discovering and loading plugin modules |
| `cli` | Command parsing, formatting, and exit code behavior |

## How it works

1. The library modules talk directly to Linux interfaces such as `/proc`, `/sys`, `/dev`, `ioctl`, and `getifaddrs`.
2. Each fallible public API returns `nizaw::Result<T>` with structured error information.
3. The CLI consumes these APIs and prints either human-readable text or JSON.
4. Plugins are discovered from a directory and loaded dynamically as shared objects.

## Error handling

Expected failures such as missing files, permission errors, or vanished processes are represented with `nizaw::Result<T>` instead of exceptions. This keeps the public API predictable and script-friendly.

## Dependency policy

The default posture is zero required third-party runtime dependencies. The project prefers the C++20 standard library and direct Linux APIs over external packages where possible.
