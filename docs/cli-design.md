# Nizaw — CLI Design

Status: Stable command-line interface design

## 1. Command hierarchy

```
nizaw
├── system
│   └── info
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
│   └── info <DEVICE>
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
│   └── capabilities
│
└── plugins
    └── list [DIRECTORY]
```

Each leaf command maps 1:1 onto a library call (see `api-design.md` §2) —
the CLI layer's job is: parse args → call library → format output → set
exit code. No leaf command computes anything the library doesn't expose.

## 2. Global flags

```
--help / -h        show help and exit
--version          show version and exit
--verbose / -v     increase log verbosity
--quiet            suppress non-essential output
--json / -j        emit machine-readable JSON instead of human-readable text
--no-color         disable ANSI color in human-readable output
```

Global flags are parsed before subcommand dispatch and apply uniformly; no
subcommand is allowed to redefine their meaning.

## 3. Human-readable output style

Aligned label/value table, no ASCII art, minimal color (used only for status
words like `active`/`inactive`, never for structural decoration):

```
$ nizaw system info

Hostname:       workstation
Kernel:         Linux
Kernel release: 6.14.x
Kernel version: #1 SMP ...
Architecture:   x86_64
Uptime:         2d 04h 12m
Boot time:      2026-07-29 08:11:00
Page size:      4096
CPU count:      8
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
  "uptime": "2d 04h 12m",
  "boot_time": "2026-07-29 08:11:00",
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

## 5. Argument parsing

The CLI uses a hand-rolled minimal parser. This was chosen because the
command tree is shallow and uniform (module → subcommand → optional single
positional arg), which is exactly the case a minimal hand-rolled parser
handles well without pulling in a dependency.

## 6. Exit codes

The CLI uses the following exit codes:

- `0` — success
- `1` — general error (library call returned an `Error`)
- `2` — invalid arguments or unknown/incomplete command

Mapping from `nizaw::ErrorCode` (library) to exit codes is handled in the
`cli` module — domain modules never know about exit codes, only about
`ErrorCode`.

## 7. Privileged operations

Selected modules (`storage`, `service`, etc.) are read-only. No CLI
command performs a destructive or privilege-escalating action in the current
release. If/when `service start|stop|enable|disable` are added
(explicitly deferred — see `architecture.md` §4 Non-goals), they will
require an interactive confirmation prompt by default and a `--yes` flag to
skip it non-interactively, plus a capability/permission pre-check that fails
closed rather than attempting the syscall and letting the kernel reject it.