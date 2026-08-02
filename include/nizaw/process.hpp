#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <sys/types.h>

#include "nizaw/result.hpp"

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

[[nodiscard]] Result<std::vector<ProcessInfo>> list();
[[nodiscard]] Result<ProcessInfo> inspect(pid_t pid);
[[nodiscard]] Result<std::map<std::string, std::string>> environment(pid_t pid);

}  // namespace nizaw::process
