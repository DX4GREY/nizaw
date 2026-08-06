# Nizaw Wiki

Nizaw is a modern, modular Linux system and CLI framework written in C++20. It combines a reusable C++ library with a script-friendly CLI for system introspection, process inspection, filesystem and storage reporting, network information, service status, security context, and plugin discovery.

## Project purpose

Nizaw is designed for developers and system administrators who need a lightweight, Linux-native way to inspect the current machine state from C++ code or from the command line. The project aims to provide:

- a typed and predictable API for Linux system introspection
- a simple CLI that is easy to automate in scripts
- zero-friction access to system information without shelling out to many unrelated tools
- a modular foundation that can grow into a larger Linux tooling platform

## What this project is for

Nizaw is useful when you want to:

- inspect the host environment from a C++ application
- build a small admin tool without writing low-level parsing code from scratch
- collect process, filesystem, storage, network, service, and security information in one place
- create scriptable system diagnostics that work consistently across Linux distros
- build extensions or plugins on top of the same core platform

## What you can do with Nizaw

- Query host and kernel details with `nizaw system info`
- Inspect running processes with `nizaw process list` and `nizaw process inspect <PID>`
- Check filesystem usage and metadata with `nizaw fs usage <PATH>` and `nizaw fs info <PATH>`
- List storage devices with `nizaw storage list` and inspect with `nizaw storage info <DEVICE>`
- List network interfaces with `nizaw network interfaces` and inspect with `nizaw network info <IFACE>`
- List systemd services with `nizaw service list` and inspect with `nizaw service status <UNIT>`
- Get security identity with `nizaw security identity` and capabilities with `nizaw security capabilities`
- Discover and load third-party plugins with `nizaw plugins list [DIRECTORY]`
- Manage the distributed agent with `nizaw agent status`, `nizaw agent config`, `nizaw agent start`, `nizaw agent stop`
- Use the same APIs from a C++ application without going through the CLI

## Typical use cases

- Embedded diagnostics inside a daemon or service
- Host inventory and system inspection tools
- Minimal Linux monitoring utilities
- Automation scripts for deployment or troubleshooting
- Prototyping a custom system information dashboard

## Quick start

```bash
# Standard build
cmake -S . -B build -DNIZAW_BUILD_CLI=ON
cmake --build build --parallel --target nizaw
./build/nizaw --help

# Portable build (self-contained binary)
cmake -S . -B build -G Ninja \
    -DNIZAW_PORTABLE=ON \
    -DNIZAW_BUILD_CLI=ON \
    -DNIZAW_BUILD_TESTS=OFF \
    -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/nizaw --version
```

## Typical workflow

1. Build the project
2. Run one of the CLI commands
3. Integrate the headers from `include/nizaw` into your own C++ application
4. Use `nizaw::Result<T>`-based error handling for production-safe code

## Wiki pages

- [Installation](Installation)
- [Usage](Usage)
- [Architecture](Architecture)
- [Development](Development)
- [Plugin Development](PluginDevelopment)
- [Troubleshooting](Troubleshooting)
- [Integration](Integration)

## Project status

**Version:** 3.0.5

The project currently targets Linux x86_64 systems with C++20 and builds a complete modular library plus a CLI executable. The default build includes all modules (core, system, process, filesystem, storage, network, service, security, plugin, remote, agent), tests, and the `nizaw` CLI. Examples can be enabled with `-DNIZAW_BUILD_EXAMPLES=ON`.

### Available modules

- **core** - Error handling, logging, platform detection, environment helpers, version metadata
- **system** - Hostname, kernel info, uptime, boot time, CPU count, page size
- **process** - Process enumeration and inspection via /proc
- **filesystem** - Disk usage and filesystem metadata
- **storage** - Block device enumeration and metadata from /sys/block
- **network** - Interface enumeration and address reporting
- **service** - systemd unit listing and status via D-Bus
- **security** - UID/GID/group and capability reporting
- **plugin** - Plugin discovery and dynamic loading from .so files
- **remote** - Pluggable transport layer for agent-orchestrator communication (HTTP, WebSocket, MQTT, gRPC, ZeroMQ, QUIC)
- **agent** - Distributed agentic orchestration: lifecycle, config, telemetry, task execution, SQLite-backed task queue
- **cli** - Complete command-line application with human-readable and JSON output

### Build options

- `NIZAW_BUILD_CLI=ON` (default) - Build the nizaw CLI executable
- `NIZAW_BUILD_TESTS=ON` (default) - Build unit/integration tests
- `NIZAW_BUILD_EXAMPLES=OFF` - Build example programs
- `NIZAW_ENABLE_SYSTEMD=ON` (default) - Enable systemd-backed service module
- `NIZAW_BUILD_AGENT=ON` (default) - Build the nizaw agent (remote orchestration)
- `NIZAW_BUILD_SHARED=OFF` - Build libnizaw as a shared library
- `NIZAW_PORTABLE=OFF` - Build self-contained release with static third-party dependencies
- `NIZAW_TRANSPORT_WEBSOCKET=OFF` - Enable WebSocket transport
- `NIZAW_TRANSPORT_MQTT=OFF` - Enable MQTT transport
- `NIZAW_TRANSPORT_GRPC=OFF` - Enable gRPC transport
- `NIZAW_TRANSPORT_ZMQ=OFF` - Enable ZeroMQ transport
- `NIZAW_TRANSPORT_QUIC=OFF` - Enable QUIC/HTTP3 transport
