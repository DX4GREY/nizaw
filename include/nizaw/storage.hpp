#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "nizaw/result.hpp"

namespace nizaw::storage {

enum class DeviceType {
    Unknown,
    Disk,
    Partition,
    Loop,
    Ram,
};

struct Device {
    std::string name;
    std::string sys_path;
    std::string dev_node;
    std::string model;
    std::string vendor;
    std::uint64_t size_bytes = 0;
    std::uint32_t logical_block_size = 0;
    std::uint32_t physical_block_size = 0;
    bool removable = false;
    bool read_only = false;
    bool rotational = false;
    DeviceType type = DeviceType::Unknown;
};

struct IoStats {
    std::uint64_t read_ops = 0;
    std::uint64_t write_ops = 0;
    std::uint64_t read_sectors = 0;
    std::uint64_t write_sectors = 0;
    std::uint64_t read_bytes = 0;
    std::uint64_t write_bytes = 0;
    double io_time_ms = 0.0;
};

[[nodiscard]] Result<std::vector<Device>> enumerate();
[[nodiscard]] Result<Device> inspect(const std::string& device);
[[nodiscard]] Result<IoStats> iostat(const std::string& device);

}  // namespace nizaw::storage
