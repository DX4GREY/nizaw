# Nizaw — Architecture

Status: Stable architecture and implementation
Version: 1.0.2

## 1. Purpose

Nizaw is a modern, modular, Linux-native System & CLI Framework written in
C++20. It has two deliverables built from the same source tree:

1. **`libnizaw`** — a library that other C++ developers link against to query
   and interact with a Linux system (processes, storage, filesystem, network,
   services, security context) through a typed, `Result<T>`-based API.
2. **`nizaw`** — a CLI application that is itself just a consumer of
   `libnizaw`. The CLI has no privileged knowledge the library doesn't expose.

This split is the single most important architectural decision in the
project: **the CLI must never contain logic that the library doesn't have.**
If a CLI command needs data, the library gains an API for it first, and the
CLI becomes a thin formatter over that API. This guarantees the library is
never an afterthought and always stays independently useful/embeddable.

## 2. Target users

- Developers building Linux system tools/daemons in C++ who want typed,
  RAII-friendly access to `/proc`, `/sys`, `/dev`, and related kernel
  interfaces without hand-rolling parsers.
- Sysadmins / power users who want a single, consistent, scriptable CLI
  (human-readable and `--json`) instead of stitching together `ps`, `lsblk`,
  `ip`, `systemctl`, `df`, etc. with inconsistent output formats.
- Distro-agnostic tooling authors who don't want to special-case Ubuntu vs
  Arch vs Fedora for basic system introspection.

## 3. Scope

In scope:
- Read-mostly system introspection: system info, process enumeration/
  inspection, filesystem usage/info, block storage enumeration,
  network interface enumeration, systemd service listing/status, security
  identity/capabilities.
- A plugin system for third-party command extension.
- JSON output mode for all commands, for scripting.

Explicitly **out of scope** for the foreseeable roadmap (see Non-goals).

## 4. Non-goals

- **Not** a replacement for `systemd` itself, a init system, or a service
  supervisor. The `service` module *observes* systemd (and is designed so
  other backends could be added later); it does not implement one.
- **Not** a privilege-escalation or security-bypass tool. `nizaw security`
  reports on identity/capabilities; it does not grant them.
- **Not** a shell/command-execution wrapper. Nizaw does not shell out to
  `ps`, `lsblk`, `ip`, `df`, etc. If a Linux API or `/proc`/`/sys` interface
  exists for the data, that's the only sanctioned path. `system()`/`popen()`
  are disallowed by default (see Security Requirements below).
- **Not** cross-platform. Linux x86_64 is the only supported target;
  distro/kernel differences are handled behind an abstraction layer, but
  Windows/macOS/BSD are not goals.
- **Not** a full observability/monitoring stack (no persistence, no
  time-series storage, no alerting). Nizaw answers "what is the state right
  now", not "what was the state an hour ago."
- Destructive service operations (`start`/`stop`/`restart`/`enable`/
  `disable`) are explicitly deferred past initial `service` module scope and
  will require an explicit confirmation/permission design of their own
  before being added.

## 5. High-level architecture

```
                     NIZAW
                       │
              ┌────────┴────────┐
              │                 │
           Library             CLI
              │                 │
       ┌──────┼──────┐          │
       │      │      │          │
     System Process Filesystem  │
       │      │      │ Storage   │
       │      │      │    Network│
       │      │      │    Service│
       │      │      │    Security│
       │      │      │    Plugin │
       └──────┼──────┘          │
              │                 │
              └────────┬────────┘
                       │
               Linux Kernel APIs
                       │
         ┌─────────────┼─────────────┐
         │             │             │
        /proc         /sys          /dev
         │             │             │
         └─────────────┼─────────────┘
                       │
                   Linux OS
```

`libnizaw` is organized as a set of independent modules that each own one
area of system interaction. Modules depend on `nizaw::core` and, where
sensible, on each other (e.g. `service` may use `process` to resolve a unit's
main PID), but there is **no** dependency from `core` back up into any
domain module, and CLI never gets special access — it only calls public
module APIs. The current modules are:

- **core** - Error handling, logging, platform detection, environment helpers, version metadata
- **system** - Hostname, kernel info, uptime, boot time, CPU count, page size
- **process** - Process enumeration and inspection via /proc
- **filesystem** - Disk usage and filesystem metadata
- **storage** - Block device enumeration and metadata from /sys/block
- **network** - Interface enumeration and address reporting
- **service** - systemd unit listing and status via D-Bus
- **security** - UID/GID/group and capability reporting
- **plugin** - Plugin discovery and dynamic loading from .so files
- **cli** - Complete command-line application with human-readable and JSON output

## 6. Module boundaries

| Module        | Owns                                                                 | Depends on           |
|---------------|-----------------------------------------------------------------------|-----------------------|
| `core`        | `Result<T>`, `Error`/`ErrorCode`, logging, version metadata, platform detection, environment utilities | (nothing internal)   |
| `system`      | Hostname, kernel info, arch, uptime, boot time, CPU count, page size  | `core`                |
| `process`     | `/proc`-based process enumeration & inspection                        | `core`                |
| `filesystem`  | `std::filesystem`-based usage/info, mount points, permissions          | `core`                |
| `storage`     | Block device enumeration via `/sys/block`, `/dev`, ioctl               | `core`                |
| `network`     | Interface enumeration via `getifaddrs`/ioctl/netlink, sysfs            | `core`                |
| `service`     | systemd unit listing/status via D-Bus (with a backend-abstraction seam) | `core`                |
| `security`    | UID/GID/groups/capabilities/privilege introspection                    | `core`                |
| `plugin`      | Dynamic command loading (`.so`), API versioning, plugin registry       | `core`                |
| `cli`         | Argument parsing, command tree, formatting (human + JSON), exit codes  | all of the above (as needed per command) |

Each module is a **separate CMake target** so consumers can link only what
they need (e.g. a daemon that only wants `nizaw::process` shouldn't have to
pull in `nizaw::network`).

## 7. Dependency policy (summary — full detail in `dependency-policy.md`)

Default is zero third-party runtime dependencies. The C++20 standard library
and Linux kernel APIs are preferred over any library. The CLI uses a
hand-rolled minimal argument parser (see `cli-design.md` §5). systemd D-Bus
interaction uses `sd-bus` (part of `libsystemd`) rather than a hand-rolled
D-Bus client, because a from-scratch D-Bus implementation is a
correctness/security risk disproportionate to the module's value; this makes
`libsystemd` an **optional** dependency gating only the `service` module.

## 8. Error handling architecture

No exceptions for expected/recoverable failure paths (missing file,
permission denied, ESRCH on a vanished process, unsupported feature on a
given device, etc.). Exceptions are reserved for genuine programmer errors
(e.g. `std::logic_error`-class bugs) and are not part of the public
error-flow contract.

All fallible public APIs return `nizaw::Result<T>`, modeled as a
`std::variant`-backed, `std::expected`-like type (C++20 does not have
`std::expected`, so Nizaw provides its own minimal implementation — see
`api-design.md` §1 for the exact shape). An `Error` carries:

- `ErrorCode` (a scoped enum, stable across versions once released)
- a human-readable `message`
- a `source` / module tag (e.g. `"storage"`, `"process"`)
- an optional `errno` value when the failure originated from a syscall

Errors are never swallowed. Any internal function that ignores a possible
failure must justify it in a code comment; CI/lint policy will flag unchecked
syscalls.

## 9. Logging

`nizaw::core::log` provides `TRACE`/`DEBUG`/`INFO`/`WARN`/`ERROR`/`FATAL`
levels via a small header-only implementation (no third-party logging
dependency — see `dependency-policy.md` for the reasoning). Log output never
includes secrets, credentials, or full environment dumps. The CLI's
`--verbose`/`--quiet` flags map onto the logger's level filter.

## 10. Threading model

The modules are designed to be **synchronous and single-threaded by
default** — process/storage/network enumeration are inherently bounded,
one-shot filesystem/syscall operations, and introducing concurrency there
would add complexity without a measured performance justification.
`std::thread`/`std::mutex` are reserved for cases with a demonstrated need,
e.g. a future `--watch` mode or the plugin loader guarding a shared registry.
Where module state exists, it is instance-owned (no global mutable state),
so callers can already parallelize across independent module calls if they
choose.

## 11. Memory management

RAII throughout; smart pointers (`std::unique_ptr` primarily) for owned
dynamic resources such as loaded plugin handles; raw pointers only for
non-owning views. File descriptors, `DIR*` handles from `/proc` iteration,
and `dlopen` handles are always wrapped in RAII guards so an early `return`
from a `Result`-returning function can never leak a resource.

## 12. API stability strategy

Pre-1.0 (`v0.x`), the public API is allowed to break between minor versions,
but every breaking change must be called out in `CHANGELOG.md`. From `v1.0.0`
onward, Nizaw follows semantic versioning on the public headers in
`include/nizaw/`: patch = bug fixes only, minor = additive/non-breaking API,
major = breaking. ABI stability (safe to swap the shared library without
recompiling consumers) is a stated **goal for a post-1.0 milestone**, not a
See `api-design.md` §5 for what that will require
(pimpl-heavy public classes, no virtual dispatch across the ABI boundary,
frozen struct layouts, etc.).

## 13. Resolved design decisions

These were flagged as open during early design and have since been resolved:

1. CLI argument parsing: a hand-rolled minimal parser was chosen over a
   third-party library, because the command tree is shallow and uniform
   (see `cli-design.md` §5).
2. The `service` module uses `libsystemd`/`sd-bus` as an optional,
   feature-gated dependency (`NIZAW_ENABLE_SYSTEMD`, default ON). When
   unavailable, a stub implementation is built so the rest of the project
   still compiles (see `dependency-policy.md` §4).
