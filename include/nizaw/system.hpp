#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "nizaw/result.hpp"

namespace nizaw::system {

struct CpuInfo {
    std::string model_name;
    std::string vendor_id;
    std::size_t frequency_mhz = 0;
    std::size_t cache_size_kb = 0;
    std::size_t core_count = 0;
};

struct LoadAverage {
    double one_min = 0.0;
    double five_min = 0.0;
    double fifteen_min = 0.0;
};

struct MemoryInfo {
    std::uint64_t total_kb = 0;
    std::uint64_t free_kb = 0;
    std::uint64_t available_kb = 0;
    std::uint64_t buffers_kb = 0;
    std::uint64_t cached_kb = 0;
    std::uint64_t shared_kb = 0;
};

struct SwapInfo {
    std::uint64_t total_kb = 0;
    std::uint64_t used_kb = 0;
    std::uint64_t free_kb = 0;
};

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
[[nodiscard]] Result<std::vector<CpuInfo>> cpu_info();
[[nodiscard]] Result<MemoryInfo> memory_info();
[[nodiscard]] Result<LoadAverage> load_average();
[[nodiscard]] Result<SwapInfo> swap_info();
[[nodiscard]] Result<std::vector<std::string>> kernel_modules();

}  // namespace nizaw::system
