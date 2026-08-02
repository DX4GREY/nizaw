#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <vector>

#include "nizaw/result.hpp"

namespace nizaw::core {

/// Options that control the behavior of write/mutation operations.
///
/// All write operations in nizaw accept these options to provide consistent
/// safety controls across the entire API surface.
struct WriteOptions {
    /// If true, the operation is only validated but not executed.
    /// Useful for "what-if" scenarios and testing.
    bool dry_run = false;

    /// If true, skip interactive confirmation prompts.
    /// Use with caution - this is for automation scripts.
    bool force = false;

    /// For operations that affect multiple items (e.g., recursive delete),
    /// apply the operation recursively.
    bool recursive = false;

    /// Optional timeout for long-running operations.
    /// If the operation doesn't complete within this duration, it fails.
    std::optional<std::chrono::seconds> timeout;

    /// If set, the user must explicitly confirm with this exact message
    /// before the operation proceeds. If empty, no confirmation is requested.
    std::optional<std::string> confirm_prompt;
};

/// Linux capability checks for security-conscious operations.
///
/// Many write operations require specific Linux capabilities.
/// This class provides a way to check if the current process has them.
class CapabilitySet {
public:
    CapabilitySet() = default;

    /// Check if the current process has the specified capability.
    [[nodiscard]] bool check(int capability) const noexcept;

    /// Convenience checks for common capability combinations.
    [[nodiscard]] bool has_admin() const noexcept;        // CAP_SYS_ADMIN
    [[nodiscard]] bool has_network_admin() const noexcept; // CAP_NET_ADMIN
    [[nodiscard]] bool has_dac_override() const noexcept;  // CAP_DAC_OVERRIDE
    [[nodiscard]] bool has_setuid() const noexcept;        // CAP_SETUID
    [[nodiscard]] bool has_setgid() const noexcept;        // CAP_SETGID
    [[nodiscard]] bool has_kill() const noexcept;          // CAP_KILL
    [[nodiscard]] bool has_sys_ptrace() const noexcept;    // CAP_SYS_PTRACE
    [[nodiscard]] bool has_sys_time() const noexcept;      // CAP_SYS_TIME
    [[nodiscard]] bool has_setpcap() const noexcept;       // CAP_SETPCAP
    [[nodiscard]] bool has_setfcaps() const noexcept;      // CAP_SETFCAP

    /// Returns true if running as root (UID 0).
    [[nodiscard]] bool is_root() const noexcept;

    /// Returns the real UID of the current process.
    [[nodiscard]] uid_t real_uid() const noexcept;

    /// Returns the effective UID of the current process.
    [[nodiscard]] uid_t effective_uid() const noexcept;

public:
    /// Get the capability set for the current process (convenience function).
    static CapabilitySet from_current() noexcept;

private:
    mutable uid_t uid_ = 0;
    mutable uid_t euid_ = 0;
    mutable std::vector<int> capabilities_;
    mutable bool capabilities_loaded_ = false;

    void load_capabilities() const noexcept;
};

/// Get the capability set for the current process.
[[nodiscard]] CapabilitySet current_capabilities() noexcept;

/// Audit logging for write operations.
///
/// Provides structured logging of all mutating operations for security
/// and compliance purposes.
class AuditLogger {
public:
    static AuditLogger& instance() noexcept;

    /// Log a write operation attempt.
    void log(const std::string& module,
             const std::string& operation,
             const std::string& target,
             bool success,
             const std::string& details = {});

    /// Set the minimum log level for audit messages.
    void set_level(int level) noexcept;

private:
    AuditLogger() = default;
    int level_ = 0;  // 0 = off, higher = more verbose
};

}  // namespace nizaw::core