# Integrating Nizaw into Your Project

This page shows how to use Nizaw from another C++ project.

## 1. Add Nizaw as a dependency

There are two common approaches:

- build Nizaw from source in the same workspace
- link your project against the built libraries from this repository

For a simple setup, the second option is usually easiest.

## 2. CMake example

If your project already uses CMake, add Nizaw as a subdirectory or as a prebuilt dependency.

### Option A: add as subdirectory

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_app LANGUAGES CXX)

add_subdirectory(/path/to/nizaw nizaw_build)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE nizaw::system nizaw::process)
```

### Option B: link to the built targets

```cmake
cmake_minimum_required(VERSION 3.20)
project(my_app LANGUAGES CXX)

find_library(NIZAW_SYSTEM_LIB nizaw_system REQUIRED)
find_library(NIZAW_CORE_LIB nizaw_core REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app PRIVATE ${NIZAW_CORE_LIB} ${NIZAW_SYSTEM_LIB})
```

## 3. Example application

```cpp
#include <iostream>
#include <nizaw/system.hpp>
#include <nizaw/process.hpp>

int main() {
    auto sys = nizaw::system::info();
    if (!sys) {
        std::cerr << "Failed to read system info: " << sys.error().message() << '\n';
        return 1;
    }

    std::cout << "Hostname: " << sys.value().hostname << '\n';

    auto procs = nizaw::process::list();
    if (!procs) {
        std::cerr << "Failed to list processes: " << procs.error().message() << '\n';
        return 1;
    }

    std::cout << "Processes found: " << procs.value().size() << '\n';
    return 0;
}
```

## 4. Build and run

```bash
cmake -S . -B build
cmake --build build
./build/my_app
```

## 5. Notes

- Use `nizaw::Result<T>` for fallible operations.
- Prefer the library API for real application integration instead of calling the CLI from inside your program.
- The CLI is mainly intended for manual use and scripting.
