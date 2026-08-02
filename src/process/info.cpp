#include "nizaw/process.hpp"

#include "nizaw/core/log.hpp"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <signal.h>
#include <sstream>
#include <string>
#include <string_view>
#include <sys/resource.h>
#include <system_error>
#include <unistd.h>

namespace nizaw::process {
namespace {

std::string trim(std::string value) {
    const auto start = value.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return {};
    }
    const auto end = value.find_last_not_of(" \t\n\r");
    return value.substr(start, end - start + 1);
}

std::optional<long long> parse_ll(std::string_view value) {
    try {
        return std::stoll(std::string(value));
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::optional<std::uint64_t> parse_u64(std::string_view value) {
    try {
        return static_cast<std::uint64_t>(std::stoull(std::string(value)));
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::string format_timestamp(std::time_t timestamp) {
    std::tm tm_value{};
#ifdef _WIN32
    gmtime_s(&tm_value, &timestamp);
#else
    gmtime_r(&timestamp, &tm_value);
#endif
    std::ostringstream out;
    out << std::put_time(&tm_value, "%Y-%m-%d %H:%M:%S UTC");
    return out.str();
}

std::string read_command_line(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        return {};
    }
    std::string buffer((std::istreambuf_iterator<char>(input)), {});
    std::replace(buffer.begin(), buffer.end(), '\0', ' ');
    return trim(buffer);
}

std::string read_executable(const std::filesystem::path& base_path) {
    std::error_code ec;
    const auto executable_path = base_path / "exe";
    if (!std::filesystem::exists(executable_path, ec)) {
        return {};
    }
    if (ec) {
        return {};
    }
    try {
        const auto resolved = std::filesystem::read_symlink(executable_path, ec);
        if (ec) {
            return {};
        }
        return resolved.string();
    } catch (const std::exception&) {
        return {};
    }
}

}  // namespace

Result<std::vector<ProcessInfo>> list() {
    const std::filesystem::path proc_dir("/proc");
    std::error_code ec;
    if (!std::filesystem::exists(proc_dir, ec) || ec) {
        return Error(ErrorCode::NotFound, "/proc is unavailable", "process");
    }

    std::vector<ProcessInfo> processes;
    for (const auto& entry : std::filesystem::directory_iterator(proc_dir, ec)) {
        if (ec) {
            break;
        }
        if (!entry.is_directory(ec)) {
            continue;
        }
        const std::string name = entry.path().filename().string();
        if (!std::all_of(name.begin(), name.end(), [](char ch) { return std::isdigit(ch); })) {
            continue;
        }

        const auto pid = static_cast<pid_t>(std::stoi(name));
        auto inspect_result = inspect(pid);
        if (inspect_result) {
            processes.push_back(inspect_result.value());
        }
    }

    return processes;
}

Result<ProcessInfo> inspect(pid_t pid) {
    ProcessInfo info;
    info.pid = pid;

    const auto base_path = std::filesystem::path("/proc") / std::to_string(pid);
    if (!std::filesystem::exists(base_path)) {
        return Error(ErrorCode::NotFound, "Process no longer exists", "process");
    }

    const auto status_path = base_path / "status";
    std::ifstream status_file(status_path);
    if (!status_file) {
        if (errno == EACCES) {
            return Error(ErrorCode::PermissionDenied, "Permission denied while reading process status", "process");
        }
        return Error::from_errno(errno, ErrorCode::IoError, "process", "failed to read process status");
    }

    std::string line;
    while (std::getline(status_file, line)) {
        const auto separator = line.find(':');
        if (separator == std::string::npos) {
            continue;
        }
        const std::string key = trim(line.substr(0, separator));
        const std::string value = trim(line.substr(separator + 1));

        if (key == "Name") {
            info.name = value;
        } else if (key == "State") {
            info.state = value;
        } else if (key == "PPid") {
            if (const auto parsed = parse_ll(value)) {
                info.ppid = static_cast<pid_t>(*parsed);
            }
        } else if (key == "Uid") {
            const auto fields = value.find('\t');
            if (fields != std::string::npos) {
                if (const auto parsed = parse_ll(value.substr(0, fields))) {
                    info.uid = static_cast<uid_t>(*parsed);
                }
            }
        } else if (key == "Gid") {
            const auto fields = value.find('\t');
            if (fields != std::string::npos) {
                if (const auto parsed = parse_ll(value.substr(0, fields))) {
                    info.gid = static_cast<gid_t>(*parsed);
                }
            }
        } else if (key == "Threads") {
            if (const auto parsed = parse_u64(value)) {
                info.threads = static_cast<std::size_t>(*parsed);
            }
        } else if (key == "VmRSS") {
            if (const auto parsed = parse_u64(value)) {
                info.memory_kb = *parsed;
            }
        }
    }

    const auto comm_path = base_path / "comm";
    if (std::ifstream comm_file(comm_path); comm_file) {
        std::string comm_name;
        std::getline(comm_file, comm_name);
        if (!comm_name.empty()) {
            info.name = trim(comm_name);
        }
    }

    info.command = read_command_line(base_path / "cmdline");
    if (info.command.empty() && !info.name.empty()) {
        info.command = info.name;
    }
    info.arguments = info.command;
    info.executable = read_executable(base_path);

    const auto stat_path = base_path / "stat";
    std::ifstream stat_file(stat_path);
    if (stat_file) {
        std::string stat_content((std::istreambuf_iterator<char>(stat_file)), {});
        std::stringstream stream(stat_content);
        std::string field;
        std::vector<std::string> fields;
        while (std::getline(stream, field, ' ')) {
            fields.push_back(field);
        }
        if (fields.size() > 13) {
            const auto utime = parse_ll(fields[12]);
            const auto stime = parse_ll(fields[13]);
            const auto start_ticks = parse_ll(fields[20]);
            const auto ticks_per_second = sysconf(_SC_CLK_TCK);
            if (utime && stime && start_ticks && ticks_per_second > 0) {
                info.cpu_time_seconds = static_cast<double>(*utime + *stime) / static_cast<double>(ticks_per_second);
            }
            if (start_ticks) {
                std::ifstream uptime_file("/proc/uptime");
                std::string uptime_line;
                if (std::getline(uptime_file, uptime_line)) {
                    const auto uptime_seconds = std::stod(uptime_line.substr(0, uptime_line.find(' ')));
                    std::ifstream proc_stat_file("/proc/stat");
                    std::string line2;
                    while (std::getline(proc_stat_file, line2)) {
                        if (line2.rfind("btime ", 0) == 0) {
                            const auto boot_time = std::stoll(line2.substr(6));
                            const auto start_seconds = static_cast<double>(boot_time) + uptime_seconds -
                                                       static_cast<double>(*start_ticks) / static_cast<double>(ticks_per_second);
                            info.start_time = format_timestamp(static_cast<std::time_t>(start_seconds));
                            break;
                        }
                    }
                }
            }
        }
    }

    return info;
}

Result<std::map<std::string, std::string>> environment(pid_t pid) {
    const auto environ_path = std::filesystem::path("/proc") / std::to_string(pid) / "environ";
    if (!std::filesystem::exists(environ_path)) {
        return Error(ErrorCode::NotFound, "Process no longer exists", "process");
    }

    std::ifstream environ_file(environ_path, std::ios::binary);
    if (!environ_file) {
        if (errno == EACCES) {
            return Error(ErrorCode::PermissionDenied, "Permission denied while reading process environment", "process");
        }
        return Error::from_errno(errno, ErrorCode::IoError, "process", "failed to read process environment");
    }

    std::map<std::string, std::string> env;
    std::string buffer((std::istreambuf_iterator<char>(environ_file)), {});
    
    std::size_t start = 0;
    while (start < buffer.size()) {
        const auto equal_pos = buffer.find('=', start);
        if (equal_pos == std::string::npos) {
            break;
        }
        
        const std::string key = buffer.substr(start, equal_pos - start);
        const auto value_start = equal_pos + 1;
        const auto null_pos = buffer.find('\0', value_start);
        const std::string value = (null_pos != std::string::npos) 
            ? buffer.substr(value_start, null_pos - value_start)
            : buffer.substr(value_start);
        
        env[key] = value;
        start = (null_pos != std::string::npos) ? null_pos + 1 : buffer.size();
    }

    return env;
}

// Write operations implementation

Result<void> send_signal(pid_t pid, int signal, const core::WriteOptions& options) {
    if (options.dry_run) {
        NIZAW_LOG_INFO("process", "Would send signal {} to PID {}", signal, pid);
        return {};
    }

    if (options.confirm_prompt) {
        NIZAW_LOG_WARN("process", "Confirmation required: {}", *options.confirm_prompt);
        return Error(ErrorCode::ConfirmationRequired, 
                     "Confirmation required for signal", "process");
    }

    if (::kill(pid, signal) != 0) {
        if (errno == ESRCH) {
            return Error(ErrorCode::NotFound, "Process does not exist", "process");
        }
        if (errno == EPERM) {
            return Error(ErrorCode::PermissionDenied, 
                         "Permission denied to send signal to process", "process");
        }
        return Error::from_errno(errno, ErrorCode::IoError, "process",
                                "kill() failed");
    }

    NIZAW_LOG_INFO("process", "Sent signal {} to PID {}", signal, pid);
    core::AuditLogger::instance().log("process", "send_signal", 
                                      std::to_string(pid), true,
                                      std::string(std::format("signal={}", signal)));
    return {};
}

Result<void> terminate(pid_t pid, const core::WriteOptions& options) {
    const int signal = options.force ? SIGKILL : SIGTERM;
    const std::string signal_name = options.force ? "SIGKILL" : "SIGTERM";
    
    if (options.dry_run) {
        NIZAW_LOG_INFO("process", "Would terminate PID {} with {}", pid, signal_name);
        return {};
    }

    if (options.confirm_prompt) {
        NIZAW_LOG_WARN("process", "Confirmation required: {}", *options.confirm_prompt);
        return Error(ErrorCode::ConfirmationRequired, 
                     "Confirmation required for terminate", "process");
    }

    if (::kill(pid, signal) != 0) {
        if (errno == ESRCH) {
            return Error(ErrorCode::NotFound, "Process does not exist", "process");
        }
        if (errno == EPERM) {
            return Error(ErrorCode::PermissionDenied, 
                         "Permission denied to terminate process", "process");
        }
        return Error::from_errno(errno, ErrorCode::IoError, "process",
                                "kill() failed");
    }

    NIZAW_LOG_INFO("process", "Terminated PID {} with {}", pid, signal_name);
    core::AuditLogger::instance().log("process", "terminate", 
                                      std::to_string(pid), true,
                                      std::string(std::format("signal={}", signal_name)));
    return {};
}

Result<void> suspend(pid_t pid, const core::WriteOptions& options) {
    if (options.dry_run) {
        NIZAW_LOG_INFO("process", "Would suspend PID {}", pid);
        return {};
    }

    if (options.confirm_prompt) {
        NIZAW_LOG_WARN("process", "Confirmation required: {}", *options.confirm_prompt);
        return Error(ErrorCode::ConfirmationRequired, 
                     "Confirmation required for suspend", "process");
    }

    if (::kill(pid, SIGSTOP) != 0) {
        if (errno == ESRCH) {
            return Error(ErrorCode::NotFound, "Process does not exist", "process");
        }
        if (errno == EPERM) {
            return Error(ErrorCode::PermissionDenied, 
                         "Permission denied to suspend process", "process");
        }
        return Error::from_errno(errno, ErrorCode::IoError, "process",
                                "kill(SIGSTOP) failed");
    }

    NIZAW_LOG_INFO("process", "Suspended PID {}", pid);
    core::AuditLogger::instance().log("process", "suspend", std::to_string(pid), true);
    return {};
}

Result<void> resume(pid_t pid, const core::WriteOptions& options) {
    if (options.dry_run) {
        NIZAW_LOG_INFO("process", "Would resume PID {}", pid);
        return {};
    }

    if (options.confirm_prompt) {
        NIZAW_LOG_WARN("process", "Confirmation required: {}", *options.confirm_prompt);
        return Error(ErrorCode::ConfirmationRequired, 
                     "Confirmation required for resume", "process");
    }

    if (::kill(pid, SIGCONT) != 0) {
        if (errno == ESRCH) {
            return Error(ErrorCode::NotFound, "Process does not exist", "process");
        }
        if (errno == EPERM) {
            return Error(ErrorCode::PermissionDenied, 
                         "Permission denied to resume process", "process");
        }
        return Error::from_errno(errno, ErrorCode::IoError, "process",
                                "kill(SIGCONT) failed");
    }

    NIZAW_LOG_INFO("process", "Resumed PID {}", pid);
    core::AuditLogger::instance().log("process", "resume", std::to_string(pid), true);
    return {};
}

Result<void> set_nice(pid_t pid, int nice_value, const core::WriteOptions& options) {
    if (options.dry_run) {
        NIZAW_LOG_INFO("process", "Would set nice value of PID {} to {}", pid, nice_value);
        return {};
    }

    if (options.confirm_prompt) {
        NIZAW_LOG_WARN("process", "Confirmation required: {}", *options.confirm_prompt);
        return Error(ErrorCode::ConfirmationRequired, 
                     "Confirmation required for set_nice", "process");
    }

    if (nice_value < -20 || nice_value > 19) {
        return Error(ErrorCode::InvalidArgument, 
                     "Nice value must be between -20 and 19", "process");
    }

    if (::setpriority(PRIO_PROCESS, pid, nice_value) != 0) {
        if (errno == ESRCH) {
            return Error(ErrorCode::NotFound, "Process does not exist", "process");
        }
        if (errno == EPERM) {
            return Error(ErrorCode::PermissionDenied, 
                         "Permission denied to set nice value", "process");
        }
        return Error::from_errno(errno, ErrorCode::IoError, "process",
                                "setpriority() failed");
    }

    NIZAW_LOG_INFO("process", "Set nice value of PID {} to {}", pid, nice_value);
    core::AuditLogger::instance().log("process", "set_nice", 
                                      std::to_string(pid), true,
                                      std::string(std::format("nice={}", nice_value)));
    return {};
}

}  // namespace nizaw::process
