#pragma once

#include <string>

namespace nizaw::core {

/// Best-effort description of the distro/init-system Nizaw is running on.
///
/// This is deliberately *not* `nizaw::system::SystemInfo` (Phase 2) — that
/// module answers "what is this machine's identity" for display purposes.
/// `PlatformInfo` answers "which code path should the abstraction layer
/// take", e.g. "does this distro's D-Bus expose systemd, so the `service`
/// module can use its systemd backend". Distro fields are read from
/// `/etc/os-release` (the freedesktop.org standard present on all currently
/// targeted distros — Ubuntu, Debian, Arch, Fedora, Kali).
struct PlatformInfo {
    /// Lowercase machine-readable distro id, e.g. "ubuntu", "debian",
    /// "arch", "fedora", "kali". Empty if `/etc/os-release` is missing or
    /// doesn't define ID (never guessed/hardcoded).
    std::string distro_id;

    /// Human-readable distro name, e.g. "Ubuntu 24.04.1 LTS".
    std::string distro_name;

    /// Distro version string, e.g. "24.04". Empty if not present (rolling
    /// releases such as Arch typically omit this).
    std::string distro_version;

    /// True if this system appears to be running under systemd, detected
    /// via the presence of `/run/systemd/system` (the documented systemd
    /// detection convention) rather than assumed.
    bool has_systemd = false;
};

/// Detects platform information. This never fails outright — if
/// `/etc/os-release` can't be read, the returned `PlatformInfo` simply has
/// empty distro fields rather than the call returning an `Error`, since
/// "unknown distro" is a valid, handleable state for callers rather than a
/// fatal condition.
[[nodiscard]] PlatformInfo detect() noexcept;

}  // namespace nizaw::core
