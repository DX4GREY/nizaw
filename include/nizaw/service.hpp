#pragma once

#include <optional>
#include <string>
#include <vector>

#include "nizaw/result.hpp"
#include "nizaw/core/write.hpp"

namespace nizaw::service {

struct ServiceInfo {
    std::string name;
    std::string description;
    std::string load_state;
    std::string active_state;
    std::string sub_state;
    bool loaded = false;
    bool active = false;
    std::optional<bool> enabled;
    std::optional<pid_t> main_pid;
};

// Read operations
[[nodiscard]] Result<std::vector<ServiceInfo>> list();
[[nodiscard]] Result<ServiceInfo> inspect(std::string_view unit_name);

// Write operations

/// Service control actions.
enum class ServiceAction {
    Start,
    Stop,
    Restart,
    Reload
};

/// Control a systemd service.
/// @param unit_name The service unit name (e.g., "nginx.service")
/// @param action The action to perform
/// @param options WriteOptions for safety controls
Result<void> control(std::string_view unit_name,
                     ServiceAction action,
                     const core::WriteOptions& options = {});

/// Enable a service to start at boot.
/// @param unit_name The service unit name
/// @param options WriteOptions
Result<void> enable(std::string_view unit_name,
                    const core::WriteOptions& options = {});

/// Disable a service from starting at boot.
/// @param unit_name The service unit name
/// @param options WriteOptions
Result<void> disable(std::string_view unit_name,
                     const core::WriteOptions& options = {});

}  // namespace nizaw::service
