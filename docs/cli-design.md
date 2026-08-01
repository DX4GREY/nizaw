# Nizaw — CLI Design

Status: Phase 0 — Draft for review

## 1. Command hierarchy

```
nizaw
├── system
│   ├── info
│   ├── uptime
│   ├── hostname
│   └── kernel
│
├── process
│   ├── list
│   └── inspect <PID>
│
├── fs
│   ├── usage <PATH>
│   └── info <PATH>
│
├── storage
│   ├── list
│   ├── info <DEVICE>
│   └── health <DEVICE>
│
├── network
│   ├── interfaces
│   └── info <IFACE>
│
├── service
│   ├── list
│   ├── status <UNIT>
│   └── inspect <UNIT>
│
├── security
│   ├── identity
│   ├── capabilities
│   └── check
│
└── plugins
    └── list
```

Each leaf command maps 1:1 onto a library call (see `api-design.md` §2) —
the CLI layer's job is: parse args → call library → format output → set
exit code. No leaf command computes anything the library doesn't expose.

## 2. Global flags

```
--help              show help and exit
--version            show version and exit
--verbose            increase log verbosity (repeatable: -v, -vv)
--quiet              suppress non-essential output
--json               emit machine-readable JSON instead of human-readable text
--no-color           disable ANSI color in human-readable output
```

Global flags are parsed before subcommand dispatch and apply uniformly; no
subcommand is allowed to redefine their meaning.

## 3. Human-readable output style

Aligned label/value table, no ASCII art, minimal color (used only for status
words like `active`/`inactive`, never for structural decoration):

```
$ nizaw system info

System
────────────────────────────
Hostname       workstation
Kernel         6.14.x
Architecture   x86_64
Uptime         2d 04h 12m
CPU Cores      8
```

`--no-color` strips ANSI entirely (also auto-detected when stdout is not a
TTY, matching common CLI convention — `NO_COLOR` env var is also honored).

## 4. JSON output

Every command supports `--json`. Shape mirrors the corresponding library
struct field-for-field (snake_case keys), so JSON output is effectively a
direct serialization of the `Result<T>` payload:

```bash
$ nizaw system info --json
```
```json
{
  "hostname": "workstation",
  "kernel_name": "Linux",
  "kernel_release": "6.14.0",
  "kernel_version": "#1 SMP ...",
  "architecture": "x86_64",
  "uptime_seconds": 187920,
  "boot_time": "2026-07-29T08:11:00Z",
  "page_size": 4096,
  "cpu_count": 8
}
```

On error, JSON mode emits a single error object to stdout (not stderr, so
`| jq` pipelines still work) instead of the human-readable error line:

```json
{
  "error": {
    "code": "PermissionDenied",
    "message": "Permission denied: this operation requires elevated privileges.",
    "source": "storage"
  }
}
```

## 5. Argument parsing: open decision

Two options, to be settled before Phase 9 (not blocking Phase 0–8 library
work):

**Option A — hand-rolled minimal parser** (recommended default)
- Pros: zero dependency, full control over `--json`/exit-code conventions,
  trivial to keep dependency-light per project policy.
- Cons: more code to write/maintain ourselves; subcommand help text
  generation has to be built by hand.

**Option B — a small header-only third-party parser** (e.g. CLI11)
- Pros: mature help/usage generation, less boilerplate.
- Cons: violates the "avoid dependency unless std lib is truly insufficient"
  policy for something a ~300-line hand-rolled parser can cover, given
  Nizaw's command tree is only 2 levels deep and flags are simple.

Recommendation: **Option A**, because the command tree is shallow and
uniform (module → subcommand → optional single positional arg), which is
exactly the case a minimal hand-rolled parser handles well without pulling
in a dependency. This will be revisited if the CLI's argument complexity
grows materially (e.g. many optional flags per command, complex validation).

## 6. Exit codes

```cpp
enum class ExitCode : int {
    Success            = 0,
    GeneralError       = 1,
    InvalidArguments    = 2,
    PermissionDenied    = 3,
    ResourceUnavailable = 4,
    Unsupported         = 5,
};
```

Mapping from `nizaw::ErrorCode` (library) to `ExitCode` (CLI) is a single,
centralized table in the `cli` module — domain modules never know about
exit codes, only about `ErrorCode`.

## 7. Privileged operations

Phase 5–7 modules (`storage health`, `service status`, etc.) are read-only.
No CLI command performs a destructive or privilege-escalating action in the
current roadmap. If/when `service start|stop|enable|disable` are added
(explicitly deferred — see `architecture.md` §4 Non-goals), they will
require an interactive confirmation prompt by default and a `--yes` flag to
skip it non-interactively, plus a capability/permission pre-check that fails
closed with `ExitCode::PermissionDenied` rather than attempting the syscall
and letting the kernel reject it.
