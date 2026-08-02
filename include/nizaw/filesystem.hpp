#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "nizaw/result.hpp"

namespace nizaw::filesystem {

struct DiskUsage {
    std::uint64_t total_bytes = 0;
    std::uint64_t free_bytes = 0;
    std::uint64_t available_bytes = 0;
    std::uint64_t used_bytes = 0;
};

struct EntryInfo {
    std::string path;
    std::string type;
    std::string permissions;
    std::string owner;
    std::string group;
    std::uintmax_t size_bytes = 0;
    std::uintmax_t inode = 0;
    bool exists = false;
    bool is_symlink = false;
    std::string mount_point;
};

struct MountPoint {
    std::string device;
    std::string mount_point;
    std::string filesystem_type;
    std::string options;
    int dump = 0;
    int pass = 0;
};

[[nodiscard]] Result<DiskUsage> usage(const std::filesystem::path& path);
[[nodiscard]] Result<EntryInfo> info(const std::filesystem::path& path);
[[nodiscard]] Result<std::vector<MountPoint>> mounts();

}  // namespace nizaw::filesystem
