#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include <sys/types.h>

#include "nizaw/result.hpp"
#include "nizaw/core/write.hpp"

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

// Write operations

/// Create a directory and all intermediate directories if needed.
Result<void> create_directory(const std::filesystem::path& path,
                              const core::WriteOptions& options = {},
                              mode_t permissions = 0755);

/// Remove a file or directory.
Result<void> remove(const std::filesystem::path& path,
                    const core::WriteOptions& options = {});

/// Rename/move a file or directory.
Result<void> rename(const std::filesystem::path& from,
                    const std::filesystem::path& to,
                    const core::WriteOptions& options = {});

/// Copy a file or directory.
Result<void> copy(const std::filesystem::path& from,
                  const std::filesystem::path& to,
                  const core::WriteOptions& options = {});

/// Set permissions on a file or directory.
Result<void> set_permissions(const std::filesystem::path& path,
                             mode_t permissions,
                             const core::WriteOptions& options = {});

/// Set owner and group of a file or directory.
Result<void> set_owner(const std::filesystem::path& path,
                       uid_t uid,
                       gid_t gid,
                       const core::WriteOptions& options = {});

/// Create a symbolic link.
Result<void> create_symlink(const std::filesystem::path& target,
                            const std::filesystem::path& link,
                            const core::WriteOptions& options = {});

/// Write content to a file (creates or overwrites).
Result<void> write_file(const std::filesystem::path& path,
                        std::string_view content,
                        const core::WriteOptions& options = {},
                        mode_t permissions = 0644);

/// Read content from a file.
Result<std::string> read_file(const std::filesystem::path& path);

/// Truncate a file to a specific size.
Result<void> truncate(const std::filesystem::path& path,
                      std::uintmax_t size,
                      const core::WriteOptions& options = {});

}  // namespace nizaw::filesystem
