# Getting Started

## Prerequisites

- Linux x86_64 system
- C++20 compiler (GCC 10+ or Clang 11+)
- CMake 3.20 or newer
- pkg-config (for optional systemd detection)
- Ninja (recommended)
- OpenSSL (for agent/remote module, required when `NIZAW_BUILD_AGENT=ON`)
- zlib (for agent/remote module, required when `NIZAW_BUILD_AGENT=ON`)
- SQLite3 (for agent module, required when `NIZAW_BUILD_AGENT=ON`)

## Quick Build

```bash
# Configure
cmake -S . -B build -G Ninja

# Build library, CLI, and tests
cmake --build build --parallel

# Run tests
ctest --test-dir build --output-on-failure

# Try the CLI
./build/nizaw --help
./build/nizaw system info

# Agent commands (requires NIZAW_BUILD_AGENT=ON)
./build/nizaw agent status
./build/nizaw agent config --validate --config agent.toml
```

## Library Usage

### Basic Example

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

### Error Handling

Always check for errors using `Result<T>`:

```cpp
auto result = nizaw::system::info();
if (!result) {
    // Handle error
    std::cerr << "Error: " << result.error().message() << '\n';
    return 1;
}

// Safe to access value
std::cout << result.value().hostname << '\n';
```

### CMake Integration

Add Nizaw to your CMake project:

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_project LANGUAGES CXX)

# Option 1: Add as subdirectory
add_subdirectory(/path/to/nizaw nizaw_build)

# Option 2: Find pre-built library
find_library(NIZAW_SYSTEM_LIB nizaw_system REQUIRED)
find_library(NIZAW_CORE_LIB nizaw_core REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE nizaw::system nizaw::core)
```

## Next Steps

- See [docs/wiki/Usage.md](docs/wiki/Usage.md) for comprehensive examples
- Read [docs/architecture.md](docs/architecture.md) to understand the design
- Check [docs/api-design.md](docs/api-design.md) for complete API reference
- Review [docs/wiki/PluginDevelopment.md](docs/wiki/PluginDevelopment.md) to create plugins
