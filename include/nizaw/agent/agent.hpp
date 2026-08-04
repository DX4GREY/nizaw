#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <cstdint>
#include <chrono>

#include "nizaw/result.hpp"

namespace nizaw::agent {

struct AgentConfig {
    std::string id;
    std::string server_url;
    std::string ca_cert;
    std::string client_cert;
    std::string client_key;
    
    std::chrono::seconds heartbeat_interval{60};
    int jitter_percent = 30;
    int max_parallel_tasks = 4;
    std::string temp_dir = "/var/cache/nizaw/";
    
    bool allow_exec = true;
    bool allow_fetch = true;
    bool allow_push = true;
    std::vector<std::string> restricted_paths;
};

struct TelemetryData {
    std::string hostname;
    std::string kernel;
    std::string arch;
    std::string uptime;
    std::string loadavg;
    std::string ip_address;
    std::string agent_version;
};

struct TaskResult {
    std::string task_id;
    bool success = false;
    std::string output;
    int exit_code = -1;
    std::string error_message;
};

using TaskId = std::string;

// Agent lifecycle
[[nodiscard]] Result<void> start_daemon(const AgentConfig& config);
[[nodiscard]] Result<void> stop_daemon();
[[nodiscard]] Result<bool> is_running();
[[nodiscard]] Result<std::string> get_pid();

// Configuration
[[nodiscard]] Result<AgentConfig> load_config(std::string_view path);
[[nodiscard]] Result<void> validate_config(const AgentConfig& config);

// Telemetry
[[nodiscard]] TelemetryData collect_telemetry();

// Foreground mode for debugging
[[nodiscard]] Result<int> run_foreground(const AgentConfig& config);

}  // namespace nizaw::agent