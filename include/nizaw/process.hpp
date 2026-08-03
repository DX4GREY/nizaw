#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <sys/types.h>

#include "nizaw/result.hpp"
#include "nizaw/core/write.hpp"

namespace nizaw::process {

struct ProcessInfo {
    pid_t pid = 0;
    pid_t ppid = 0;
    uid_t uid = 0;
    gid_t gid = 0;
    std::string name;
    std::string state;
    std::string command;
    std::string executable;
    std::string arguments;
    std::size_t threads = 0;
    std::uint64_t memory_kb = 0;
    double cpu_time_seconds = 0.0;
    std::string start_time;
    std::map<std::string, std::string> environment;
};

struct ResourceLimits {
    std::uint64_t max_cpu_time_seconds = 0;
    std::uint64_t max_file_size_bytes = 0;
    std::uint64_t max_data_size_bytes = 0;
    std::uint64_t max_stack_size_bytes = 0;
    std::uint64_t max_core_file_size_bytes = 0;
    std::uint64_t max_resident_set_bytes = 0;
    std::uint64_t max_processes = 0;
    std::uint64_t max_open_files = 0;
    std::uint64_t max_locked_memory_bytes = 0;
    std::uint64_t max_address_space_bytes = 0;
    std::uint64_t max_file_locks = 0;
    std::uint64_t max_pending_signals = 0;
    std::uint64_t max_msgqueue_size_bytes = 0;
    std::uint64_t max_nice_priority = 0;
    std::uint64_t max_realtime_priority = 0;
    std::uint64_t max_realtime_timeout_us = 0;
};

struct IoStats {
    std::uint64_t read_bytes = 0;
    std::uint64_t write_bytes = 0;
    std::uint64_t cancelled_write_bytes = 0;
    std::uint64_t syscr = 0;  // read syscalls
    std::uint64_t syscw = 0;  // write syscalls
};

// Read operations
[[nodiscard]] Result<std::vector<ProcessInfo>> list();
[[nodiscard]] Result<ProcessInfo> inspect(pid_t pid);
[[nodiscard]] Result<std::map<std::string, std::string>> environment(pid_t pid);
[[nodiscard]] Result<ResourceLimits> resource_limits(pid_t pid);
[[nodiscard]] Result<IoStats> io_stats(pid_t pid);

// Write operations

/// Send a signal to a process.
/// @param pid The target process ID
/// @param signal The signal number (e.g., SIGTERM, SIGKILL, SIGSTOP)
/// @param options WriteOptions for safety controls
Result<void> send_signal(pid_t pid, int signal,
                         const core::WriteOptions& options = {});

/// Terminate a process gracefully (SIGTERM by default).
/// @param pid The target process ID
/// @param options WriteOptions - use force for SIGKILL
Result<void> terminate(pid_t pid,
                       const core::WriteOptions& options = {});

/// Suspend a process (SIGSTOP).
/// @param pid The target process ID
/// @param options WriteOptions
Result<void> suspend(pid_t pid,
                     const core::WriteOptions& options = {});

/// Resume a suspended process (SIGCONT).
/// @param pid The target process ID
/// @param options WriteOptions
Result<void> resume(pid_t pid,
                    const core::WriteOptions& options = {});

/// Change the nice value of a process.
/// @param pid The target process ID
/// @param nice_value New nice value (-20 to 19, lower = higher priority)
/// @param options WriteOptions
Result<void> set_nice(pid_t pid, int nice_value,
                      const core::WriteOptions& options = {});

}  // namespace nizaw::process
