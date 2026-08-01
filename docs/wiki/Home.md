# Nizaw Wiki

Nizaw is a modern, modular Linux system and CLI framework written in C++20. It combines a reusable C++ library with a script-friendly CLI for system introspection, process inspection, storage and network reporting, service metadata, security context, and plugin discovery.

## What you can do with Nizaw

- Query host and kernel details with `nizaw system info`
- Inspect running processes with `nizaw process list` and `nizaw process inspect <PID>`
- Check filesystem usage and metadata with `nizaw fs usage <PATH>` and `nizaw fs info <PATH>`
- Enumerate storage devices and network interfaces
- Inspect service units and security identity/capabilities
- Load third-party plugins from `.so` files

## Quick start

```bash
cmake -S . -B build
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/nizaw --help
```

## Wiki pages

- [Installation](Installation)
- [Usage](Usage)
- [Architecture](Architecture)
- [Development](Development)
- [Troubleshooting](Troubleshooting)

## Project status

The project currently targets Linux systems with C++20 and builds a modular library plus a CLI executable. The default build includes the core modules, tests, and the `nizaw` CLI.
