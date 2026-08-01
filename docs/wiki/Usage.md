# Usage

## CLI usage

The CLI entry point is `nizaw` and follows a simple command tree.

### Global flags

- `--help` / `-h` show usage
- `--version` show version info
- `--json` / `-j` emit JSON output
- `--verbose` / `-v` increase log verbosity
- `--quiet` suppress non-essential output
- `--no-color` disable ANSI color output

### Commands

#### System

```bash
./build/nizaw system info
./build/nizaw --version
```

#### Process

```bash
./build/nizaw process list
./build/nizaw process inspect 1234
```

#### Filesystem

```bash
./build/nizaw fs usage /
./build/nizaw fs info /
```

#### Storage

```bash
./build/nizaw storage list
./build/nizaw storage info sda
```

#### Network

```bash
./build/nizaw network interfaces
./build/nizaw network info eth0
```

#### Service

```bash
./build/nizaw service list
./build/nizaw service status ssh
./build/nizaw service inspect ssh
```

#### Security

```bash
./build/nizaw security identity
./build/nizaw security capabilities
```

#### Plugins

```bash
./build/nizaw plugins list
./build/nizaw plugins list ./plugins
```

### JSON output

```bash
./build/nizaw system info --json
./build/nizaw process list --json
```

## Library usage example

```cpp
#include <iostream>
#include <nizaw/system.hpp>

int main() {
    auto result = nizaw::system::info();
    if (!result) {
        std::cerr << result.error().message() << '\n';
        return 1;
    }

    const auto& info = result.value();
    std::cout << "Hostname: " << info.hostname << '\n';
    std::cout << "Kernel:   " << info.kernel_release << '\n';
    std::cout << "Arch:     " << info.architecture << '\n';
    return 0;
}
```

Compile this example by linking against the project targets in the same build tree:

```bash
cmake -S . -B build -DNIZAW_BUILD_EXAMPLES=ON
cmake --build build --parallel --target nizaw_example
```

## Core module usage

The `nizaw::core` module provides error handling, logging, platform detection, environment helpers, and version metadata.

```cpp
#include <iostream>
#include <nizaw/core/platform.hpp>
#include <nizaw/core/version.hpp>

int main() {
    const auto version = nizaw::core::version();
    std::cout << "Nizaw " << version.major << "." << version.minor << "." << version.patch << '\n';

    const auto plat = nizaw::core::detect();
    std::cout << "Distro: " << plat.distro_name << " (" << plat.distro_id << ")\n";
    std::cout << "Systemd: " << (plat.has_systemd ? "yes" : "no") << '\n';
    return 0;
}
```

## Typical use cases

- Embedded diagnostics inside a Linux daemon
- Host inspection in deployment scripts
- Monitoring and inventory tools
- Lightweight system introspection for C++ applications

## Who should use this project

This project is most useful for:

- C++ developers who need Linux system information in a typed API
- system administrators who want a simple CLI for quick inspection
- DevOps or SRE teams building lightweight diagnostics tools
- students and hobbyists learning how Linux system APIs can be wrapped in C++

## Module-by-module summary

- `core` → error handling, logging, platform detection, environment helpers, version metadata
- `system` → host and kernel facts
- `process` → running process detail
- `filesystem` → disk usage and file metadata
- `storage` → block devices and storage characteristics
- `network` → interfaces and addressing
- `service` → service state and unit visibility
- `security` → execution identity and capabilities
- `plugin` → dynamic extension support