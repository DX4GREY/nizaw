# Contributing to Nizaw

Contributions are welcome. Please keep changes incremental and aligned with the architecture in [docs/architecture.md](docs/architecture.md).

## Development workflow

1. Create a focused branch for your work.
2. Keep public API changes documented in [docs/api-design.md](docs/api-design.md).
3. Add or update tests for behavior changes.
4. Run the project build and test suite before submitting a change.

## Before contributing

- Read [docs/architecture.md](docs/architecture.md) to understand the module boundaries and design principles
- Review [docs/api-design.md](docs/api-design.md) for the current public API surface
- Check [CHANGELOG.md](CHANGELOG.md) for recent changes and version history
- See [docs/dependency-policy.md](docs/dependency-policy.md) for guidelines on adding dependencies

## Code conventions

- Use `nizaw::Result<T>` for all fallible public APIs
- Keep public headers free from platform-specific implementation details
- Avoid shelling out to external commands when a Linux API exists
- Keep the CLI thin; put logic in the library layer
- Use `NIZAW_LOG_*` macros for diagnostics instead of raw `std::cerr` in library code
- Use `nizaw::Error::from_errno` when wrapping syscall failures
- All new modules must have tests under `tests/<module>/`

## Build and test

```bash
# Configure with all options enabled
cmake -S . -B build -G Ninja -DNIZAW_BUILD_CLI=ON -DNIZAW_BUILD_TESTS=ON -DNIZAW_BUILD_EXAMPLES=ON

# Build everything
cmake --build build --parallel

# Run tests
ctest --test-dir build --output-on-failure -V

# Run the CLI to verify
./build/nizaw --help
./build/nizaw system info
```

## Adding a new module

1. Create a header in `include/nizaw/<module>.hpp`
2. Implement in `src/<module>/info.cpp`
3. Register the target in `CMakeLists.txt` following the existing pattern
4. Add tests under `tests/<module>/test_<module>.cpp`
5. Update [docs/api-design.md](docs/api-design.md) with the new API signatures
6. Add CLI commands in `src/cli/cli.cpp` if user-facing
7. Update documentation in [docs/wiki/Usage.md](docs/wiki/Usage.md)

## Adding write operations

Write operations must:
- Accept `const core::WriteOptions& options = {}` as the last parameter
- Return `Result<T>` (typically `Result<void>`)
- Include confirmation prompts for destructive actions
- Check capabilities when appropriate
- Use `AuditLogger::instance().log()` for audit trail

## Pull request checklist

- [ ] Code compiles without warnings (`-Wall -Wextra -Wpedantic -Wshadow`)
- [ ] All tests pass
- [ ] New APIs are documented in `docs/api-design.md`
- [ ] New features have usage examples in `docs/wiki/Usage.md`
- [ ] CHANGELOG.md is updated
- [ ] No new third-party dependencies (see [docs/dependency-policy.md](docs/dependency-policy.md))
