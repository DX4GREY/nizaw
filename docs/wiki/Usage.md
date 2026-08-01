# Usage

## CLI usage

The CLI entry point is `nizaw` and follows a simple command tree.

### Global flags

- `--help` show usage
- `--version` show version info
- `--json` emit JSON output
- `--verbose` increase log verbosity
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
    return 0;
}
```

Compile this example by linking against the project targets in the same build tree.

## Typical use cases

- Embedded diagnostics inside a Linux daemon
- Host inspection in deployment scripts
- Monitoring and inventory tools
- Lightweight system introspection for C++ applications
