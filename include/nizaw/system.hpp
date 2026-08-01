#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

#include "nizaw/result.hpp"

namespace nizaw::system {

struct SystemInfo {
    std::string hostname;
    std::string kernel_name;
    std::string kernel_release;
    std::string kernel_version;
    std::string architecture;
    std::string uptime;
    std::string boot_time;
    std::size_t page_size = 0;
    std::size_t cpu_count = 0;
};

[[nodiscard]] Result<SystemInfo> info();

}  // namespace nizaw::system
