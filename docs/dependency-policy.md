# Nizaw — Dependency Policy

Status: Phase 0 — Draft for review

## 1. Default posture

Zero required third-party runtime dependencies. Before adding *any*
dependency, the following questions must be answered in the PR/commit that
introduces it:

1. Is the C++20 standard library actually insufficient for this?
2. Does a Linux kernel API / `/proc` / `/sys` interface already provide this
   without a library?
3. What is the dependency's license, and is it compatible with Nizaw's
   license?
4. What is its maintenance status (last release, open issues, bus factor)?
5. What is its footprint (binary size, transitive dependencies, build-time
   cost)?
6. Is it required for the feature to exist at all, or optional (i.e. can the
   feature degrade gracefully / be compiled out without it)?

## 2. Standard library first

Preferred over any third-party equivalent for: filesystem operations
(`std::filesystem`), string handling (`std::string_view`, `std::format`),
optional/variant types (`std::optional`, `std::variant`), time
(`std::chrono`), concurrency primitives (`std::thread`, `std::mutex`) if/when
actually needed.

## 3. Linux API first, shell command never

If a feature can be implemented via a syscall, `/proc`, `/sys`, `/dev`,
`ioctl`, or netlink, that is the only sanctioned implementation — not a
wrapper around a shell command (`ps`, `lsblk`, `ip`, `df`, `systemctl`
output-parsing, etc.). This is a correctness and security requirement, not
just a style preference: shell-command wrapping is fragile (locale-dependent
output, version skew across distros) and reintroduces exactly the kind of
shell-injection / TOCTOU surface the project's security requirements forbid.

## 4. Currently identified dependencies

| Dependency | Module    | Required or optional | Reasoning |
|------------|-----------|----------------------|-----------|
| `libsystemd` (`sd-bus`) | `service` | Optional, feature-gated by `NIZAW_ENABLE_SYSTEMD` (default ON on systemd distros, buildable OFF) | systemd status/listing requires D-Bus. Hand-rolling a D-Bus client is a large, security-sensitive undertaking (message framing, authentication) disproportionate to what `sd-bus` already solves correctly and stably as part of the base systemd install on all target distros except where systemd itself is absent. |
| Test framework (candidate: Catch2, header-only) | `tests` only, never shipped in `libnizaw`/`nizaw` binaries | Optional, dev-only | Needed for Phase 1 onward's unit tests; does not affect the shipped library/CLI dependency footprint at all since it's test-only. Final selection deferred to Phase 1 when test infrastructure is actually built. |

No dependency is currently identified for: `core`, `system`, `process`,
`filesystem`, `storage`, `network`, `security`, `cli`, `plugin`. These are
expected to remain dependency-free, built entirely on the C++20 standard
library and direct Linux kernel interfaces.

## 5. CLI argument parsing

Not yet finalized — tracked as an open decision in `cli-design.md` §5, with
a hand-rolled minimal parser as the current recommendation specifically to
avoid adding a dependency for a shallow, uniform command tree.

## 6. Logging

No third-party logging library. `dependency-policy` disallows adding one
because Nizaw's logging needs (6 levels, stderr/stdout sink, basic
formatting) don't exceed what a ~100-line internal implementation covers,
and pulling in something like spdlog would add a real (non-header-only)
build dependency for a solved problem.

## 7. Plugin loading

`dlopen`/`dlfcn.h` (part of glibc, not a "dependency" in the policy sense —
it's a base-system Linux API) is used for the `.so`-based plugin system in
Phase 9. No plugin-framework library is used.

## 8. Revisiting this policy

This document is not frozen. Any addition or removal of a dependency must
update this table in the same commit that changes `CMakeLists.txt`'s
`find_package`/`FetchContent` calls, so this file always reflects the actual
build graph.
