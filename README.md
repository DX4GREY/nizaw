# Nizaw

Modern Linux System & CLI Framework written in C++20

> **Status:** Stable v2.0.1 — Production-ready modular library with comprehensive CLI support, write operations, and integrated tests.

## Features

### Core Foundation
- Typed `Result<T>`-based error propagation for Linux-facing APIs (no exceptions for expected failures)
- Comprehensive error codes: Unknown, NotFound, PermissionDenied, InvalidArgument, Unsupported, IoError, ParseError, ResourceUnavailable, AlreadyExists, CapabilityRequired, OperationNotPermitted, ResourceBusy, WouldBlock, InvalidState, PartialFailure, ConfirmationRequired
- Lightweight core with error handling (`nizaw::Error`, `nizaw::ErrorCode`), logging (`nizaw::core::Logger`), platform detection (`/etc/os-release`), environment helpers, and version metadata
- Zero required third-party runtime dependencies (C++20 standard library + Linux APIs only)

### System Module (`nizaw::system`)
- Host and kernel information: hostname, kernel name/release/version, architecture
- Uptime and boot time reporting
- CPU information via `/proc/cpuinfo`: model, vendor, frequency, cache size, core count
- Memory usage via `/proc/meminfo`: total, free, available, buffers, cached, shared
- Swap usage: total, used, free
- Load averages (1/5/15 minute) via `/proc/loadavg`
- Loaded kernel modules listing via `/proc/modules`
- Hardware monitoring (hwmon): temperature, fan speed, voltage, power consumption

### Process Module (`nizaw::process`)
- Process enumeration with PID, PPID, UID, GID, name, state, threads, memory, CPU time
- Detailed process inspection: command line, executable path, arguments, start time
- Process environment variable inspection
- **Write operations**: send_signal, terminate, suspend, resume, set_nice
- Graceful handling of vanished processes (ESRCH) without crashing

### Filesystem Module (`nizaw::filesystem`)
- Disk usage reporting: total, free, available, used bytes
- File/directory metadata: type, permissions, owner, group, size, inode, existence, symlink status
- Mount point enumeration via `/proc/mounts`
- **Write operations**: create_directory (with recursive support), remove, rename, copy, set_permissions (chmod), set_owner (chown), create_symlink, write_file, read_file, truncate

### Storage Module (`nizaw::storage`)
- Block device enumeration from `/sys/block` and `/sys/block/*/device`
- Device metadata: model, vendor, size, logical/physical block sizes
- Device flags: removable, read-only, rotational
- Device type classification: Disk, Partition, Loop, Ram
- I/O statistics: read/write operations, sectors, bytes, I/O time

### Network Module (`nizaw::network`)
- Network interface enumeration: name, index, state, MAC address, MTU, flags
- IPv4/IPv6 address reporting with netmask and broadcast
- Detailed interface inspection with all addresses
- Network connection listing with protocol, local/remote addresses and ports, UID, inode

### Service Module (`nizaw::service`)
- systemd unit listing via D-Bus (optional `libsystemd` dependency)
- Service inspection: name, description, load_state, active_state, sub_state, enabled, main_pid
- **Write operations**: control (start/stop/restart/reload), enable, disable
- Graceful fallback to stub implementation when systemd is unavailable

### Security Module (`nizaw::security`)
- Process identity: real/effective UID/GID, supplemental groups
- Root detection
- Effective Linux capabilities listing (human-readable names)

### Plugin Module (`nizaw::plugin`)
- Dynamic plugin discovery and loading from `.so` files
- Plugin registry with automatic lifecycle management
- API versioning (`nizaw-plugin-v1`) for compatibility
- Plugin descriptor with name, version, description, commands
- Optional lifecycle hooks: init, cleanup, execute, version, commands

### Write Operations Safety (v2.0.0+)
- `WriteOptions`: dry-run, force, recursive, timeout, confirmation prompts
- `CapabilitySet`: Check Linux capabilities (CAP_SYS_ADMIN, CAP_NET_ADMIN, CAP_DAC_OVERRIDE, etc.)
- `AuditLogger`: Structured logging for all mutating operations
- All write operations return `Result<T>` for consistent error handling
- Confirmation prompts by default for destructive operations

### Complete CLI Application
- Human-readable and JSON output modes (`--json` / `-j`)
- Global flags: `--help`, `--version`, `--verbose`, `--quiet`, `--no-color`, `--yes`
- Comprehensive command tree covering all modules
- Write command support with safety flags
- Testable build layout suitable for incremental module expansion

## Architecture

Nizaw is built on a modular architecture where the CLI is a thin consumer of the library. Every CLI command maps directly to a library API call — the CLI never contains logic that the library doesn't expose.

See [docs/architecture.md](docs/architecture.md) for the full design,
[docs/api-design.md](docs/api-design.md) for public API signatures,
[docs/cli-design.md](docs/cli-design.md) for the CLI command tree, and
[docs/dependency-policy.md](docs/dependency-policy.md) for the dependency policy.

## Requirements

- Linux x86_64 (Ubuntu, Debian, Arch, Fedora, Kali tested)
- C++20 compiler (GCC 10+ or Clang 11+)
- CMake ≥ 3.20
- pkg-config (for optional systemd detection)
- Ninja (recommended for faster builds)

## Building

### Quick Build (Library + CLI + Tests)

```bash
cmake -S . -B build -G Ninja
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/nizaw --help
```

### Build Options

| CMake Option | Default | Description |
| --- | --- | --- |
| `NIZAW_BUILD_CLI` | `ON` | Build the `nizaw` CLI executable |
| `NIZAW_BUILD_TESTS` | `ON` | Build unit/integration tests |
| `NIZAW_BUILD_EXAMPLES` | `OFF` | Build example programs |
| `NIZAW_BUILD_SHARED` | `OFF` | Build `libnizaw` as a shared library |
| `NIZAW_ENABLE_WARNINGS` | `ON` | Enable strict compiler warnings (`-Wall -Wextra -Wpedantic -Wshadow`) |
| `NIZAW_ENABLE_ASAN` | `OFF` | Enable AddressSanitizer |
| `NIZAW_ENABLE_UBSAN` | `OFF` | Enable UndefinedBehaviorSanitizer |
| `NIZAW_ENABLE_SYSTEMD` | `ON` | Enable systemd/sd-bus backed `nizaw::service` |

### Build Examples

```bash
cmake -S . -B build -G Ninja -DNIZAW_BUILD_EXAMPLES=ON
cmake --build build --parallel --target nizaw_example
./build/examples/nizaw_example
./build/examples/nizaw_example --json
```

### Build with Sanitizers

```bash
cmake -S . -B build -G Ninja -DNIZAW_ENABLE_ASAN=ON -DNIZAW_ENABLE_UBSAN=ON
cmake --build build --parallel
```

## Quick Start

### C++ Library

```cpp
#include <iostream>
#include <nizaw/system.hpp>

int main() {
    auto result = nizaw::system::info();
    if (!result) {
        std::cerr << result.error().message() << '\n';
        return 1;
    }

    std::cout << result.value().hostname << '\n';
    return 0;
}
```

### CLI

```bash
# Build with CLI enabled (enabled by default)
cmake -S . -B build -G Ninja
cmake --build build --parallel --target nizaw

# View system information
./build/nizaw system info

# View CPU information
./build/nizaw system cpu

# View memory and swap usage
./build/nizaw system memory

# View load averages
./build/nizaw system load

# List loaded kernel modules
./build/nizaw system modules

# View hardware sensors
./build/nizaw system hwmon

# List running processes
./build/nizaw process list

# Inspect a specific process
./build/nizaw process inspect 1234

# View process environment variables
./build/nizaw process environment 1234

# Send signal to process
./build/nizaw process signal 1234 SIGTERM

# Terminate a process
./build/nizaw process terminate 1234
./build/nizaw process terminate 1234 --force  # SIGKILL

# Suspend/resume process
./build/nizaw process suspend 1234
./build/nizaw process resume 1234

# Change process priority
./build/nizaw process nice 1234 -5

# Check filesystem usage
./build/nizaw fs usage /home

# Get filesystem info
./build/nizaw fs info /home/user/file.txt

# List all mount points
./build/nizaw fs mounts

# Create directory
./build/nizaw fs mkdir /tmp/test --recursive
./build/nizaw fs mkdir /tmp/test --mode 0755

# Remove file/directory
./build/nizaw fs rm /tmp/test --recursive

# Copy file/directory
./build/nizaw fs cp /path/from /path/to
./build/nizaw fs cp -r /dir/from /dir/to

# Rename/move
./build/nizaw fs mv /old/path /new/path

# Write content to file
./build/nizaw fs write /path/to/file "Hello, World!"

# Set permissions
./build/nizaw fs chmod /path/to/file 0644

# Set owner
./build/nizaw fs chown /path/to/file 1000:1000

# List storage devices
./build/nizaw storage list

# Get storage device details
./build/nizaw storage info sda

# Get storage I/O stats
./build/nizaw storage iostat sda

# List network interfaces
./build/nizaw network interfaces

# Get network interface details
./build/nizaw network info eth0

# List network connections
./build/nizaw network connections

# List systemd services
./build/nizaw service list

# Check service status
./build/nizaw service status ssh
./build/nizaw service inspect ssh

# Start/stop/restart service
./build/nizaw service start ssh
./build/nizaw service stop ssh
./build/nizaw service restart ssh
./build/nizaw service reload ssh

# Enable/disable service at boot
./build/nizaw service enable ssh
./build/nizaw service disable ssh

# Get security identity
./build/nizaw security identity

# List capabilities
./build/nizaw security capabilities

# Discover plugins
./build/nizaw plugins list ./path/to/plugins

# Output as JSON
./build/nizaw --json system info
./build/nizaw process list --json
```

## Development

See [docs/architecture.md](docs/architecture.md) for the full design and module boundaries.

## Documentation

Comprehensive documentation is available in multiple formats:

### Main Documentation (`docs/`)
- [architecture.md](docs/architecture.md) — System architecture and design principles
- [api-design.md](docs/api-design.md) — Complete API reference with signatures
- [cli-design.md](docs/cli-design.md) — CLI command tree and design
- [dependency-policy.md](docs/dependency-policy.md) — Dependency management policy
- [getting-started.md](docs/getting-started.md) — Quick start guide

### Wiki (`docs/wiki/`)
- [Home](docs/wiki/Home.md) — Project overview
- [Installation](docs/wiki/Installation.md) — Detailed installation instructions
- [Usage](docs/wiki/Usage.md) — Comprehensive usage examples
- [Architecture](docs/wiki/Architecture.md) — Module overview and design
- [Development](docs/wiki/Development.md) — Contributing and build instructions
- [Plugin Development](docs/wiki/PluginDevelopment.md) — Creating custom plugins
- [Troubleshooting](docs/wiki/Troubleshooting.md) — Common issues and solutions
- [Integration](docs/wiki/Integration.md) — Integrating Nizaw into other projects

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

The repository is currently prepared for a future explicit license choice.
