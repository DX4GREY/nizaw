# Nizaw

Modern Linux System & CLI Framework written in C++20

> **Status:** Phase 0 — architecture and specification only. No buildable
> modules exist yet. See `docs/architecture.md` for the full design and the
> phased roadmap.

## Features (planned, by phase)

- Typed, `Result<T>`-based system introspection library (`libnizaw`):
  system info, process enumeration/inspection, filesystem usage, block
  storage enumeration and health, network interfaces, systemd services,
  security/privilege identity.
- A `nizaw` CLI built entirely on top of `libnizaw` — no logic exists in
  the CLI that isn't also available as a library call.
- Human-readable and `--json` output for every command.
- A `.so`-based plugin system for third-party commands.

## Architecture

See [`docs/architecture.md`](docs/architecture.md) for the full design,
[`docs/api-design.md`](docs/api-design.md) for public API signatures,
[`docs/cli-design.md`](docs/cli-design.md) for the CLI command tree, and
[`docs/dependency-policy.md`](docs/dependency-policy.md) for the dependency
policy.

## Requirements

- Linux x86_64 (Ubuntu, Debian, Arch, Fedora, Kali currently targeted)
- C++20 compiler (GCC or Clang)
- CMake ≥ 3.20, Ninja (recommended)

## Installation / Building

Not yet available — Phase 1 (Core Foundation) introduces the first
buildable target. Once available:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build
```

## Quick Start / CLI Usage / Library Usage / Plugin Development

To be written once the corresponding phases (1, 2–8, 9) land.

## Development

This project is being built incrementally, phase by phase, per
`docs/architecture.md`. Each phase adds one module, its tests, its CLI
commands, and its documentation before the next phase begins.

## Testing

See `tests/` (populated starting Phase 1).

## Contributing

See `CONTRIBUTING.md` (to be added alongside Phase 1).

## License

**TBD** — not yet decided. This is a project-owner decision to be made
explicitly rather than assumed; flagging here rather than defaulting to a
license silently.
