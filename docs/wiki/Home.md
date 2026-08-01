# Nizaw Wiki

Nizaw is a modern, modular Linux system and CLI framework written in C++20. It combines a reusable C++ library with a script-friendly CLI for system introspection, process inspection, storage and network reporting, service metadata, security context, and plugin discovery.

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
- collect process, filesystem, storage, network, and service information in one place
- create scriptable system diagnostics that work consistently across Linux distros
- build extensions or plugins on top of the same core platform

## What you can do with Nizaw

- Query host and kernel details with `nizaw system info`
- Inspect running processes with `nizaw process list` and `nizaw process inspect <PID>`
- Check filesystem usage and metadata with `nizaw fs usage <PATH>` and `nizaw fs info <PATH>`
- Enumerate storage devices and network interfaces
- Inspect service units and security identity/capabilities
- Load third-party plugins from `.so` files
- Use the same APIs from a C++ application without going through the CLI

## Typical use cases

- Embedded diagnostics inside a daemon or service
- Host inventory and system inspection tools
- Minimal Linux monitoring utilities
- Automation scripts for deployment or troubleshooting
- Prototyping a custom system information dashboard

## Quick start

```bash
cmake -S . -B build -DNIZAW_BUILD_CLI=ON
cmake --build build --parallel --target nizaw
./build/nizaw --help
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
- [Troubleshooting](Troubleshooting)
- [Integration](Integration)

## Project status

The project currently targets Linux systems with C++20 and builds a modular library plus a CLI executable. The default build includes the core modules, tests, and the `nizaw` CLI. Examples can be enabled with `-DNIZAW_BUILD_EXAMPLES=ON`.