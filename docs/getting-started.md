# Getting Started

## Build

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## Library usage

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
