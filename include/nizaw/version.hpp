#pragma once

#include <cstdint>
#include <string>

namespace nizaw::core {

/// Compile-time version constants, kept in one place so `CMakeLists.txt`'s
/// `project(nizaw VERSION ...)` and this header never drift apart (the
/// build generates this header's values in a later phase via
/// `configure_file`; for Phase 1 they are set directly here to match
/// `CMakeLists.txt`'s `project()` call).
inline constexpr unsigned kVersionMajor = 1;
inline constexpr unsigned kVersionMinor = 0;
inline constexpr unsigned kVersionPatch = 1;

/// Human-readable version string, e.g. "1.0.1".
[[nodiscard]] std::string version_string();

struct Version {
    unsigned major;
    unsigned minor;
    unsigned patch;
};

/// Returns the current library version as a structured value.
[[nodiscard]] Version version() noexcept;

}  // namespace nizaw::core
