#pragma once

#include <optional>
#include <string>
#include <vector>

#include "nizaw/result.hpp"

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

[[nodiscard]] Result<std::vector<ServiceInfo>> list();
[[nodiscard]] Result<ServiceInfo> inspect(std::string_view unit_name);

}  // namespace nizaw::service
