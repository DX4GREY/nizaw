# Development

## Repository layout

```text
CMakeLists.txt
README.md
include/nizaw/        # public headers
src/                  # implementation files
src/cli/              # CLI entry point and dispatch
tests/                # unit and integration tests
```

## Build locally

```bash
cmake -S . -B build -G Ninja -DNIZAW_BUILD_CLI=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

## Add a new module

1. Create a new header in `include/nizaw/`
2. Implement the module in `src/<module>/`
3. Register the target in `CMakeLists.txt`
4. Add tests under `tests/<module>/`
5. Link the module from the CLI or other consumers as needed

## Coding conventions

- Prefer `nizaw::Result<T>` for fallible public APIs
- Keep public headers free from platform-specific implementation details
- Avoid shelling out to external commands when a Linux API exists
- Keep the CLI thin; put logic in the library layer
- Favor clear module boundaries over cross-module shortcuts

## Testing

The test suite is built by default. You can run specific checks with:

```bash
ctest --test-dir build --output-on-failure -V
```

## Recommended workflow

```bash
cmake -S . -B build -DNIZAW_BUILD_CLI=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/nizaw --help
```

## Contributing

Contributions are welcome. Keep changes focused, add tests for behavioral changes, and update the documentation when public behavior or build steps change.
