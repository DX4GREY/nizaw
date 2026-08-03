# Nizaw — CLI Design

Status: Stable command-line interface design

## 1. Command hierarchy

```
nizaw
├── system
│   ├── info
│   ├── cpu
│   ├── memory
│   ├── load
│   ├── modules
│   └── hwmon
│
├── process
│   ├── list
│   ├── inspect <PID>
│   ├── environment <PID>
│   ├── signal <PID> <SIGNAL>
│   ├── terminate <PID> [--force]
│   ├── suspend <PID>
│   ├── resume <PID>
│   └── nice <PID> <VALUE>
│
├── fs
│   ├── usage <PATH>
│   ├── info <PATH>
│   ├── mounts
│   ├── mkdir <PATH> [--recursive] [--mode <OCTAL>]
│   ├── rm <PATH> [--recursive]
│   ├── cp <FROM> <TO> [--recursive]
│   ├── mv <FROM> <TO>
│   ├── write <PATH> <CONTENT>
│   ├── chmod <PATH> <MODE>
│   └── chown <PATH> <UID>:<GID>
│
├── storage
│   ├── list
│   ├── info <DEVICE>
│   └── iostat <DEVICE>
│
├── network
│   ├── interfaces
│   ├── info <IFACE>
│   └── connections
│
├── service
│   ├── list
│   ├── status <UNIT>
│   ├── inspect <UNIT>
│   ├── start <UNIT>
│   ├── stop <UNIT>
│   ├── restart <UNIT>
│   ├── reload <UNIT>
│   ├── enable <UNIT>
│   └── disable <UNIT>
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
--verbose / -v     increase log verbosity (DEBUG level)
--quiet            suppress non-essential output (WARN and above only)
--json / -j        emit machine-readable JSON instead of human-readable text
--no-color         disable ANSI color in human-readable output
--yes / -y         skip interactive confirmation prompts (for write operations)
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

Flags can appear before or after the subcommand; unknown flags are passed
through to command handlers. Write operations accept optional flags like
`--force`, `--recursive`, and `--mode` depending on the operation.

## 6. Exit codes

The CLI uses the following exit codes:

- `0` — success
- `1` — general error (library call returned an `Error`)
- `2` — invalid arguments or unknown/incomplete command

Mapping from `nizaw::ErrorCode` (library) to exit codes is handled in the
`cli` module — domain modules never know about exit codes, only about
`ErrorCode`.

## 7. Privileged operations

Write operations in the `process`, `filesystem`, and `service` modules
perform privileged actions. These commands require appropriate permissions
(Linux capabilities, root, or sudo) and include safety features:

- **Confirmation prompts**: Destructive operations prompt for confirmation by default
- **Capability checks**: Operations check for required capabilities (e.g., `CAP_SYS_ADMIN` for filesystem changes)
- **`--yes` flag**: Skip confirmation prompts for automation
- **`--force` flag**: Override safety checks when necessary (use with caution)

Examples:
- `nizaw process terminate <PID>` — prompts for confirmation
- `nizaw process terminate <PID> --force` — sends SIGKILL immediately
- `nizaw fs rm /path --recursive` — prompts before recursive deletion
- `nizaw service start ssh --yes` — starts service without prompting

The CLI exits with code `1` on permission denied or capability errors,
and code `2` on invalid arguments.
