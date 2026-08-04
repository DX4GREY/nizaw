#include "nizaw/agent/agent.hpp"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <random>
#include <sstream>

#ifdef __linux__
#include <unistd.h>
#include <signal.h>
#include <sys/sysinfo.h>
#include <sys/utsname.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

namespace nizaw::agent {

namespace {

std::string generate_uuid() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::uniform_int_distribution<> dis2(0, 255);

    std::stringstream ss;
    for (int i = 0; i < 8; ++i) ss << std::hex << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; ++i) ss << std::hex << dis(gen);
    ss << "-4";
    for (int i = 0; i < 3; ++i) ss << std::hex << dis(gen);
    ss << "-";
    ss << std::hex << ((dis(gen) & 0x0f) | 0x50);
    for (int i = 0; i < 3; ++i) ss << std::hex << dis(gen);
    ss << "-";
    for (int i = 0; i < 12; ++i) ss << std::hex << dis(gen);

    return ss.str();
}

std::string get_hostname() {
    char buf[256];
    if (gethostname(buf, sizeof(buf)) == 0) {
        return buf;
    }
    return "unknown";
}

std::string get_kernel_version() {
#ifdef __linux__
    struct utsname buf;
    if (uname(&buf) == 0) {
        return std::string(buf.release);
    }
#endif
    return "unknown";
}

std::string get_architecture() {
#ifdef __linux__
    struct utsname buf;
    if (uname(&buf) == 0) {
        return buf.machine;
    }
#endif
    return "unknown";
}

std::string get_uptime() {
#ifdef __linux__
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        std::stringstream ss;
        ss << info.uptime << "s";
        return ss.str();
    }
#endif
    return "unknown";
}

std::string get_loadavg() {
#ifdef __linux__
    struct sysinfo info;
    if (sysinfo(&info) == 0) {
        std::stringstream ss;
        ss << info.loads[0] / 65536.0 << " "
           << info.loads[1] / 65536.0 << " "
           << info.loads[2] / 65536.0;
        return ss.str();
    }
#endif
    return "0.0 0.0 0.0";
}

std::string get_ip_address() {
#ifdef __linux__
    struct ifaddrs* ifap;
    if (getifaddrs(&ifap) != 0) {
        return "unknown";
    }

    std::string result = "unknown";
    for (struct ifaddrs* ifa = ifap; ifa; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET) {
            char buf[INET_ADDRSTRLEN];
            void* addr_ptr = &((struct sockaddr_in*)ifa->ifa_addr)->sin_addr;
            if (inet_ntop(AF_INET, addr_ptr, buf, sizeof(buf))) {
                if (std::string(ifa->ifa_name) != "lo") {
                    result = buf;
                    break;
                }
            }
        }
    }
    freeifaddrs(ifap);
    return result;
#else
    return "unknown";
#endif
}

std::string read_file(std::string_view path) {
    std::ifstream file(path.data());
    if (!file.is_open()) {
        return {};
    }
    std::stringstream ss;
    ss << file.rdbuf();
    return ss.str();
}

Result<AgentConfig> load_toml_config(std::string_view path) {
    AgentConfig config;
    config.id = generate_uuid();

    std::string content = read_file(path);
    if (content.empty()) {
        return Error(ErrorCode::IoError,
                     "Failed to read config file: " + std::string(path),
                     "load_config");
    }

    // Very basic TOML parser - just enough for our config
    std::istringstream iss(content);
    std::string line;
    bool in_agent_section = false;
    bool in_behavior_section = false;
    bool in_security_section = false;

    while (std::getline(iss, line)) {
        // Remove leading/trailing whitespace
        line.erase(0, line.find_first_not_of(" \t"));
        line.erase(line.find_last_not_of(" \t") + 1);

        if (line.empty() || line[0] == '#') continue;

        if (line == "[agent]") {
            in_agent_section = true;
            in_behavior_section = false;
            in_security_section = false;
            continue;
        } else if (line == "[behavior]") {
            in_agent_section = false;
            in_behavior_section = true;
            in_security_section = false;
            continue;
        } else if (line == "[security]") {
            in_agent_section = false;
            in_behavior_section = false;
            in_security_section = true;
            continue;
        } else if (line[0] == '[') {
            in_agent_section = false;
            in_behavior_section = false;
            in_security_section = false;
            continue;
        }

        size_t eq_pos = line.find('=');
        if (eq_pos == std::string::npos) continue;

        std::string key = line.substr(0, eq_pos);
        std::string value = line.substr(eq_pos + 1);

        // Trim whitespace
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        // Remove quotes
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        }

        if (in_agent_section) {
            if (key == "id" && !value.empty() && value != "auto-generate-uuid") {
                config.id = value;
            } else if (key == "server_url") {
                config.server_url = value;
            } else if (key == "ca_cert") {
                config.ca_cert = value;
            } else if (key == "client_cert") {
                config.client_cert = value;
            } else if (key == "client_key") {
                config.client_key = value;
            }
        } else if (in_behavior_section) {
            if (key == "heartbeat_interval_sec") {
                config.heartbeat_interval = std::chrono::seconds(std::stoi(value));
            } else if (key == "jitter_percent") {
                config.jitter_percent = std::stoi(value);
            } else if (key == "max_parallel_tasks") {
                config.max_parallel_tasks = std::stoi(value);
            } else if (key == "temp_dir") {
                config.temp_dir = value;
            }
        } else if (in_security_section) {
            if (key == "allow_exec") {
                config.allow_exec = (value == "true");
            } else if (key == "allow_fetch") {
                config.allow_fetch = (value == "true");
            } else if (key == "allow_push") {
                config.allow_push = (value == "true");
            } else if (key == "restricted_paths") {
                // Parse array - simplified
                if (value.size() >= 2 && value.front() == '[' && value.back() == ']') {
                    value = value.substr(1, value.size() - 2);
                    std::istringstream path_stream(value);
                    std::string path;
                    while (std::getline(path_stream, path, ',')) {
                        path.erase(0, path.find_first_not_of(" \t"));
                        path.erase(path.find_last_not_of(" \t") + 1);
                        if (!path.empty() && path.front() == '"' && path.back() == '"') {
                            path = path.substr(1, path.size() - 2);
                        }
                        if (!path.empty()) {
                            config.restricted_paths.push_back(path);
                        }
                    }
                }
            }
        }
    }

    if (config.server_url.empty()) {
        return Error(ErrorCode::InvalidArgument,
                     "server_url is required in [agent] section",
                     "load_config");
    }

    return config;
}

}  // namespace

Result<AgentConfig> load_config(std::string_view path) {
    if (path.empty() || path == "agent.toml") {
        return load_toml_config(path);
    }
    return load_toml_config(path);
}

Result<void> validate_config(const AgentConfig& config) {
    if (config.server_url.empty()) {
        return Error(ErrorCode::InvalidArgument,
                     "server_url cannot be empty",
                     "validate_config");
    }

    if (config.ca_cert.empty() || config.client_cert.empty() || config.client_key.empty()) {
        return Error(ErrorCode::InvalidArgument,
                     "TLS certificate paths must be specified",
                     "validate_config");
    }

    if (config.heartbeat_interval.count() <= 0) {
        return Error(ErrorCode::InvalidArgument,
                     "heartbeat_interval must be positive",
                     "validate_config");
    }

    if (config.jitter_percent < 0 || config.jitter_percent > 100) {
        return Error(ErrorCode::InvalidArgument,
                     "jitter_percent must be between 0 and 100",
                     "validate_config");
    }

    if (config.max_parallel_tasks < 1 || config.max_parallel_tasks > 16) {
        return Error(ErrorCode::InvalidArgument,
                     "max_parallel_tasks must be between 1 and 16",
                     "validate_config");
    }

    return {};
}

Result<void> start_daemon(const AgentConfig& config) {
    (void)config;
    return Error(ErrorCode::Unsupported,
                 "start_daemon requires systemd/cron integration",
                 "start_daemon");
}

Result<void> stop_daemon() {
    return Error(ErrorCode::Unsupported,
                 "stop_daemon requires systemd/cron integration",
                 "stop_daemon");
}

Result<bool> is_running() {
#ifdef __linux__
    std::ifstream pid_file("/var/cache/nizaw/nizawd.pid");
    if (!pid_file.is_open()) {
        return false;
    }

    std::string pid_str;
    if (!std::getline(pid_file, pid_str)) {
        return false;
    }

    pid_t pid = std::stoi(pid_str);
    if (kill(pid, 0) == 0) {
        return true;
    }
#endif
    return false;
}

Result<std::string> get_pid() {
#ifdef __linux__
    std::ifstream pid_file("/var/cache/nizaw/nizawd.pid");
    if (!pid_file.is_open()) {
        return Error(ErrorCode::IoError,
                     "PID file not found",
                     "get_pid");
    }

    std::string pid_str;
    std::getline(pid_file, pid_str);
    return pid_str;
#else
    return Error(ErrorCode::Unsupported,
                 "get_pid not implemented for this platform",
                 "get_pid");
#endif
}

TelemetryData collect_telemetry() {
    TelemetryData data;
    data.hostname = get_hostname();
    data.kernel = get_kernel_version();
    data.arch = get_architecture();
    data.uptime = get_uptime();
    data.loadavg = get_loadavg();
    data.ip_address = get_ip_address();
    data.agent_version = "3.0.0";
    return data;
}

Result<int> run_foreground(const AgentConfig& config) {
    auto validation = validate_config(config);
    if (!validation) {
        return validation.error();
    }

    // Placeholder - full implementation would start event loop here
    (void)config;
    return 0;
}

}  // namespace nizaw::agent