# Nizaw

Modern Linux System & CLI Framework written in C++20

> **Status:** Stable release candidate. The repository builds a complete modular library with CLI support and integrated tests.

## Features

- Typed `Result<T>`-based error propagation for Linux-facing APIs.
- A lightweight core foundation with error codes, logging, platform detection, environment helpers, and version metadata.
- A `nizaw::system` module that exposes host/kernel/uptime/system information via Linux APIs.
- A `nizaw::process` module that enumerates and inspects processes via /proc without crashing on vanished or permission-restricted entries.
- A `nizaw::filesystem` module that reports disk usage and filesystem metadata, including permissions, ownership, size, and symlink awareness.
- A `nizaw::storage` module that enumerates block devices from `/sys/block` and reports physical/logical block sizes, removable/read-only flags, rotational status, model, and vendor.
- A `nizaw::network` module that enumerates interfaces and reports names, indexes, state, MAC address, MTU, flags, and IPv4/IPv6 addresses.
- A `nizaw::security` module that reports process identity, supplemental groups, and effective Linux capabilities.
- A `nizaw::plugin` module that discovers and loads third-party command plugins from `.so` artifacts.
- A testable build layout suitable for incremental module expansion.

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

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md).

## License

The repository is currently prepared for a future explicit license choice.
