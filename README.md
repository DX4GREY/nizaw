# Nizaw

Modern Linux System & CLI Framework written in C++20

> **Status:** Stable release candidate with read-write support. The repository builds a complete modular library with CLI support and integrated tests.

## Features

- Typed `Result<T>`-based error propagation for Linux-facing APIs.
- A lightweight core foundation with error codes, logging, platform detection, environment helpers, and version metadata.
- A `nizaw::system` module that exposes host/kernel/uptime/system information via Linux APIs.
  - CPU information (model, vendor, frequency, cache, core count) via `/proc/cpuinfo`
  - Memory usage breakdown (total, free, available, buffers, cached, shared) via `/proc/meminfo`
  - Swap usage (total, used, free)
  - Load averages (1/5/15 minute) via `/proc/loadavg`
  - Loaded kernel modules listing via `/proc/modules`
- A `nizaw::process` module that enumerates and inspects processes via /proc without crashing on vanished processes.
  - Process listing with PID, PPID, UID, GID, name, state, memory, CPU time
  - Detailed process inspection (command line, executable path, environment)
  - **Write operations**: send_signal, terminate, suspend, resume, set_nice
- A `nizaw::filesystem` module that provides disk usage and filesystem metadata.
  - Disk usage (total, free, available, used bytes)
  - File/directory metadata (type, permissions, owner, group, size, inode)
  - Mount point enumeration via `/proc/mounts`
  - **Write operations**: create_directory, remove, rename, copy, set_permissions, set_owner, create_symlink, write_file, read_file, truncate
- A `nizaw::storage` module that enumerates block devices from `/sys/block` and reports physical/logical block sizes, removable/read-only flags, rotational status, model, and vendor.
- A `nizaw::network` module that enumerates interfaces and reports names, indexes, state, MAC address, MTU, flags, and IPv4/IPv6 addresses.
- A `nizaw::service` module that manages systemd units via D-Bus.
  - List and inspect services (load_state, active_state, enabled)
  - **Write operations**: control (start/stop/restart/reload), enable, disable
- A `nizaw::security` module that reports process identity, supplemental groups, and effective Linux capabilities.
- A `nizaw::plugin` module that discovers and loads third-party command plugins from `.so` artifacts.

### Write Operations Safety Features (New in v2.0.0)
- **WriteOptions**: dry-run, force, recursive, timeout, confirmation prompts
- **CapabilitySet**: Linux capability checking (CAP_SYS_ADMIN, CAP_NET_ADMIN, etc.)
- **AuditLogger**: Structured logging for all mutating operations
- All write operations return `Result<T>` for consistent error handling

### Complete CLI Application
- Human-readable and JSON output modes
- Write command support with safety flags
- Testable build layout suitable for incremental module expansion

## Architecture

See [docs/architecture.md](docs/architecture.md) for the full design,
[docs/api-design.md](docs/api-design.md) for public API signatures,
[docs/cli-design.md](docs/cli-design.md) for the CLI command tree, and
[docs/dependency-policy.md](docs/dependency-policy.md) for the dependency policy.

## Requirements

- Linux x86_64 (Ubuntu, Debian, Arch, Fedora, Kali currently targeted)
- C++20 compiler (GCC or Clang)
- CMake ≥ 3.20, Ninja (recommended)

## Building

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

To also build the example programs:

```bash
cmake -S . -B build -DNIZAW_BUILD_EXAMPLES=ON
cmake --build build --parallel --target nizaw_example
./build/examples/nizaw_example
```

To build the CLI application:

```bash
cmake -S . -B build -DNIZAW_BUILD_CLI=ON
cmake --build build --parallel --target nizaw
./build/nizaw --help
```

## Quick Start

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

## CLI Quick Start

```bash
# Build with CLI enabled
cmake -S . -B build -DNIZAW_BUILD_CLI=ON
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

# List running processes
./build/nizaw process list

# Inspect a specific process
./build/nizaw process inspect 1234

# View process environment variables
./build/nizaw process environment 1234

# Check filesystem usage
./build/nizaw fs usage /home

# Get filesystem info
./build/nizaw fs info /home/user/file.txt

# List all mount points
./build/nizaw fs mounts

# List storage devices
./build/nizaw storage list

# Get storage device details
./build/nizaw storage info sda

# List network interfaces
./build/nizaw network interfaces

# Get network interface details
./build/nizaw network info eth0

# List systemd services
./build/nizaw service list

# Check service status
./build/nizaw service status ssh

# Get security identity
./build/nizaw security identity

# List capabilities
./build/nizaw security capabilities

# Discover plugins
./build/nizaw plugins list ./path/to/plugins

# Output as JSON
./build/nizaw --json system info
```

## Development

See [docs/architecture.md](docs/architecture.md) for the full design and module boundaries.

## Wiki

A GitHub-Wiki-ready documentation set is available under [docs/wiki](docs/wiki), including:
- [docs/wiki/Home.md](docs/wiki/Home.md)
- [docs/wiki/Installation.md](docs/wiki/Installation.md)
- [docs/wiki/Usage.md](docs/wiki/Usage.md)
- [docs/wiki/Architecture.md](docs/wiki/Architecture.md)
- [docs/wiki/Development.md](docs/wiki/Development.md)
- [docs/wiki/Troubleshooting.md](docs/wiki/Troubleshooting.md)
- [docs/wiki/Integration.md](docs/wiki/Integration.md)

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

The repository is currently prepared for a future explicit license choice.
