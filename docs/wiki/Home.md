# Nizaw Wiki

Nizaw is a modern, modular Linux system and CLI framework written in C++20. It combines a reusable C++ library with a script-friendly CLI for system introspection, process inspection, storage and network reporting, service metadata, security context, and plugin discovery.

## What you can do with Nizaw

- Query host and kernel details with `nizaw system info`
- Inspect running processes with `nizaw process list` and `nizaw process inspect <PID>`
- Check filesystem usage and metadata with `nizaw fs usage <PATH>` and `nizaw fs info <PATH>`
- Enumerate storage devices and network interfaces
- Inspect service units and security identity/capabilities
- Load third-party plugins from `.so` files
- Use the same APIs from a C++ application without going through the CLI

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

## Project status

The project currently targets Linux systems with C++20 and builds a modular library plus a CLI executable. The default build includes the core modules, tests, and the `nizaw` CLI.
