#include "nizaw/filesystem.hpp"

#include <cerrno>
#include <cstring>
#include <fstream>
#include <grp.h>
#include <pwd.h>
#include <sstream>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "nizaw/core/log.hpp"

namespace nizaw::filesystem {
namespace {

std::string type_name(const struct stat& st) {
    if (S_ISDIR(st.st_mode)) {
        return "directory";
    }
    if (S_ISREG(st.st_mode)) {
        return "file";
    }
    if (S_ISLNK(st.st_mode)) {
        return "symlink";
    }
    if (S_ISCHR(st.st_mode)) {
        return "character-device";
    }
    if (S_ISBLK(st.st_mode)) {
        return "block-device";
    }
    if (S_ISFIFO(st.st_mode)) {
        return "fifo";
    }
    if (S_ISSOCK(st.st_mode)) {
        return "socket";
    }
    return "unknown";
}

std::string permission_string(mode_t mode) {
    const char* rwx = "rwx";
    std::string out;
    for (int i = 0; i < 3; ++i) {
        const auto shift = static_cast<int>((2 - i) * 3);
        const auto bits = (mode >> shift) & 07;
        for (int j = 0; j < 3; ++j) {
            const bool set = (bits & (1 << (2 - j))) != 0;
            out.push_back(set ? rwx[j] : '-');
        }
    }
    return out;
}

std::string owner_name(uid_t uid) {
    struct passwd* pwd = getpwuid(uid);
    if (pwd == nullptr) {
        return std::to_string(uid);
    }
    return pwd->pw_name;
}

std::string group_name(gid_t gid) {
    struct group* grp = getgrgid(gid);
    if (grp == nullptr) {
        return std::to_string(gid);
    }
    return grp->gr_name;
}

}  // namespace

Result<DiskUsage> usage(const std::filesystem::path& path) {
    std::error_code ec;
    const auto space_info = std::filesystem::space(path, ec);
    if (ec) {
        return Error::from_errno(ec.value(), ErrorCode::IoError, "filesystem",
                                 "std::filesystem::space() failed");
    }

    DiskUsage usage_info{};
    usage_info.total_bytes = static_cast<std::uint64_t>(space_info.capacity);
    usage_info.free_bytes = static_cast<std::uint64_t>(space_info.free);
    usage_info.available_bytes = static_cast<std::uint64_t>(space_info.available);
    usage_info.used_bytes = usage_info.total_bytes - usage_info.available_bytes;
    return usage_info;
}

Result<EntryInfo> info(const std::filesystem::path& path) {
    EntryInfo info{};
    info.path = path.string();

    std::error_code ec;
    const auto absolute_path = std::filesystem::absolute(path, ec);
    if (ec) {
        return Error::from_errno(ec.value(), ErrorCode::IoError, "filesystem",
                                 "std::filesystem::absolute() failed");
    }
    info.path = absolute_path.string();

    struct stat st {};
    if (lstat(absolute_path.c_str(), &st) != 0) {
        if (errno == ENOENT || errno == ENOTDIR) {
            return Error(ErrorCode::NotFound, "Path does not exist", "filesystem");
        }
        return Error::from_errno(errno, ErrorCode::IoError, "filesystem",
                                 "lstat() failed");
    }

    info.exists = true;
    info.type = type_name(st);
    info.permissions = permission_string(st.st_mode);
    info.owner = owner_name(st.st_uid);
    info.group = group_name(st.st_gid);
    info.size_bytes = static_cast<std::uintmax_t>(st.st_size);
    info.inode = static_cast<std::uintmax_t>(st.st_ino);
    info.is_symlink = S_ISLNK(st.st_mode);

    std::filesystem::path mount_path = absolute_path;
    while (!mount_path.empty() && mount_path != mount_path.parent_path()) {
        struct stat mount_stat {};
        if (stat(mount_path.c_str(), &mount_stat) == 0) {
            info.mount_point = mount_path.string();
            break;
        }
        mount_path = mount_path.parent_path();
    }

    return info;
}

Result<std::vector<MountPoint>> mounts() {
    std::ifstream mounts_file("/proc/mounts");
    if (!mounts_file) {
        return Error(ErrorCode::NotFound, "/proc/mounts is unavailable", "filesystem");
    }

    std::vector<MountPoint> mount_points;
    std::string line;
    while (std::getline(mounts_file, line)) {
        if (line.empty()) {
            continue;
        }

        std::istringstream stream(line);
        MountPoint mount{};
        if (stream >> mount.device >> mount.mount_point >> mount.filesystem_type >> mount.options) {
            if (stream >> mount.dump >> mount.pass) {
                mount_points.push_back(mount);
            }
        }
    }

    return mount_points;
}

// Write operations implementation

Result<void> create_directory(const std::filesystem::path& path,
                              const core::WriteOptions& options,
                              mode_t permissions) {
    namespace fs = std::filesystem;
    
    if (options.dry_run) {
        NIZAW_LOG_INFO("filesystem", "Would create directory: {}", path.string());
        return {};
    }

    if (options.confirm_prompt) {
        NIZAW_LOG_WARN("filesystem", "Confirmation required: {}", *options.confirm_prompt);
        return Error(ErrorCode::ConfirmationRequired, 
                     "Confirmation required for directory creation", "filesystem");
    }

    std::error_code ec;
    if (options.recursive) {
        if (!fs::create_directories(path, ec)) {
            if (ec) {
                return Error::from_errno(ec.value(), ErrorCode::IoError, "filesystem",
                                        "create_directories() failed");
            }
            // Directory already exists is not an error in recursive mode
        }
    } else {
        if (!fs::create_directory(path, ec)) {
            if (ec) {
                return Error::from_errno(ec.value(), ErrorCode::IoError, "filesystem",
                                        "create_directory() failed");
            }
            if (fs::exists(path)) {
                return Error(ErrorCode::AlreadyExists, 
                             "Directory already exists", "filesystem");
            }
        }
    }

    // Set permissions if requested
    if (permissions != 0) {
        fs::permissions(path, 
                        static_cast<fs::perms>(permissions),
                        fs::perm_options::replace, ec);
        if (ec) {
            NIZAW_LOG_WARN("filesystem", "Failed to set permissions on {}: {}", 
                          path.string(), ec.message());
        }
    }

    NIZAW_LOG_INFO("filesystem", "Created directory: {}", path.string());
    core::AuditLogger::instance().log("filesystem", "create_directory", 
                                      path.string(), true);
    return {};
}

Result<void> remove(const std::filesystem::path& path,
                    const core::WriteOptions& options) {
    namespace fs = std::filesystem;
    
    if (options.dry_run) {
        NIZAW_LOG_INFO("filesystem", "Would remove: {}", path.string());
        return {};
    }

    if (options.confirm_prompt) {
        NIZAW_LOG_WARN("filesystem", "Confirmation required: {}", *options.confirm_prompt);
        return Error(ErrorCode::ConfirmationRequired, 
                     "Confirmation required for removal", "filesystem");
    }

    std::error_code ec;
    if (options.recursive) {
        if (!fs::remove_all(path, ec)) {
            return Error::from_errno(ec.value(), ErrorCode::IoError, "filesystem",
                                    "remove_all() failed");
        }
    } else {
        if (!fs::remove(path, ec)) {
            if (ec) {
                return Error::from_errno(ec.value(), ErrorCode::IoError, "filesystem",
                                        "remove() failed");
            }
            if (!fs::exists(path)) {
                return Error(ErrorCode::NotFound, "Path does not exist", "filesystem");
            }
        }
    }

    NIZAW_LOG_INFO("filesystem", "Removed: {}", path.string());
    core::AuditLogger::instance().log("filesystem", "remove", path.string(), true);
    return {};
}

Result<void> rename(const std::filesystem::path& from,
                    const std::filesystem::path& to,
                    const core::WriteOptions& options) {
    if (options.dry_run) {
        NIZAW_LOG_INFO("filesystem", "Would rename '{}' to '{}'", from.string(), to.string());
        return {};
    }

    if (options.confirm_prompt) {
        NIZAW_LOG_WARN("filesystem", "Confirmation required: {}", *options.confirm_prompt);
        return Error(ErrorCode::ConfirmationRequired, 
                     "Confirmation required for rename", "filesystem");
    }

    std::error_code ec;
    std::filesystem::rename(from, to, ec);
    if (ec) {
        return Error::from_errno(ec.value(), ErrorCode::IoError, "filesystem",
                                "rename() failed");
    }

    NIZAW_LOG_INFO("filesystem", "Renamed '{}' to '{}'", from.string(), to.string());
    core::AuditLogger::instance().log("filesystem", "rename", 
                                      from.string() + " -> " + to.string(), true);
    return {};
}

Result<void> copy(const std::filesystem::path& from,
                  const std::filesystem::path& to,
                  const core::WriteOptions& options) {
    namespace fs = std::filesystem;
    
    if (options.dry_run) {
        NIZAW_LOG_INFO("filesystem", "Would copy '{}' to '{}'", from.string(), to.string());
        return {};
    }

    if (options.confirm_prompt) {
        NIZAW_LOG_WARN("filesystem", "Confirmation required: {}", *options.confirm_prompt);
        return Error(ErrorCode::ConfirmationRequired, 
                     "Confirmation required for copy", "filesystem");
    }

    std::error_code ec;
    if (options.recursive) {
        fs::copy(from, to, 
                 fs::copy_options::recursive | fs::copy_options::overwrite_existing,
                 ec);
        if (ec) {
            return Error::from_errno(ec.value(), ErrorCode::IoError, "filesystem",
                                    "copy() failed");
        }
    } else {
        fs::copy_file(from, to, 
                      fs::copy_options::overwrite_existing,
                      ec);
        if (ec) {
            return Error::from_errno(ec.value(), ErrorCode::IoError, "filesystem",
                                    "copy_file() failed");
        }
    }

    NIZAW_LOG_INFO("filesystem", "Copied '{}' to '{}'", from.string(), to.string());
    core::AuditLogger::instance().log("filesystem", "copy", 
                                      from.string() + " -> " + to.string(), true);
    return {};
}

Result<void> set_permissions(const std::filesystem::path& path,
                             mode_t permissions,
                             const core::WriteOptions& options) {
    if (options.dry_run) {
        NIZAW_LOG_INFO("filesystem", "Would set permissions on '{}' to {:o}", 
                      path.string(), permissions);
        return {};
    }

    if (options.confirm_prompt) {
        NIZAW_LOG_WARN("filesystem", "Confirmation required: {}", *options.confirm_prompt);
        return Error(ErrorCode::ConfirmationRequired, 
                     "Confirmation required for chmod", "filesystem");
    }

    std::error_code ec;
    std::filesystem::permissions(path, 
                                 static_cast<std::filesystem::perms>(permissions),
                                 std::filesystem::perm_options::replace, ec);
    if (ec) {
        return Error::from_errno(ec.value(), ErrorCode::IoError, "filesystem",
                                "permissions() failed");
    }

    NIZAW_LOG_INFO("filesystem", "Set permissions on '{}' to {:o}", 
                   path.string(), permissions);
    core::AuditLogger::instance().log("filesystem", "set_permissions", 
                                      path.string(), true,
                                      std::string(std::format("mode={:o}", permissions)));
    return {};
}

Result<void> set_owner(const std::filesystem::path& path,
                       uid_t uid,
                       gid_t gid,
                       const core::WriteOptions& options) {
    if (options.dry_run) {
        NIZAW_LOG_INFO("filesystem", "Would set owner of '{}' to uid={}, gid={}", 
                      path.string(), uid, gid);
        return {};
    }

    if (options.confirm_prompt) {
        NIZAW_LOG_WARN("filesystem", "Confirmation required: {}", *options.confirm_prompt);
        return Error(ErrorCode::ConfirmationRequired, 
                     "Confirmation required for chown", "filesystem");
    }

    if (::chown(path.c_str(), uid, gid) != 0) {
        return Error::from_errno(errno, ErrorCode::IoError, "filesystem",
                                "chown() failed");
    }

    NIZAW_LOG_INFO("filesystem", "Set owner of '{}' to uid={}, gid={}", 
                   path.string(), uid, gid);
    core::AuditLogger::instance().log("filesystem", "set_owner", 
                                      path.string(), true,
                                      std::string(std::format("uid={}, gid={}", uid, gid)));
    return {};
}

Result<void> create_symlink(const std::filesystem::path& target,
                            const std::filesystem::path& link,
                            const core::WriteOptions& options) {
    if (options.dry_run) {
        NIZAW_LOG_INFO("filesystem", "Would create symlink '{}' -> '{}'", 
                      link.string(), target.string());
        return {};
    }

    if (options.confirm_prompt) {
        NIZAW_LOG_WARN("filesystem", "Confirmation required: {}", *options.confirm_prompt);
        return Error(ErrorCode::ConfirmationRequired, 
                     "Confirmation required for symlink", "filesystem");
    }

    std::error_code ec;
    std::filesystem::create_symlink(target, link, ec);
    if (ec) {
        return Error::from_errno(ec.value(), ErrorCode::IoError, "filesystem",
                                "create_symlink() failed");
    }

    NIZAW_LOG_INFO("filesystem", "Created symlink '{}' -> '{}'", 
                   link.string(), target.string());
    core::AuditLogger::instance().log("filesystem", "create_symlink", 
                                      link.string(), true,
                                      "target=" + target.string());
    return {};
}

Result<void> write_file(const std::filesystem::path& path,
                        std::string_view content,
                        const core::WriteOptions& options,
                        mode_t permissions) {
    if (options.dry_run) {
        NIZAW_LOG_INFO("filesystem", "Would write {} bytes to '{}'", 
                      content.size(), path.string());
        return {};
    }

    if (options.confirm_prompt) {
        NIZAW_LOG_WARN("filesystem", "Confirmation required: {}", *options.confirm_prompt);
        return Error(ErrorCode::ConfirmationRequired, 
                     "Confirmation required for write_file", "filesystem");
    }

    // Write the content
    {
        std::ofstream file(path, std::ios::binary);
        if (!file) {
            return Error(ErrorCode::IoError, "Failed to open file for writing", "filesystem");
        }
        
        file.write(content.data(), static_cast<std::streamsize>(content.size()));
        if (!file) {
            return Error(ErrorCode::IoError, "Failed to write to file", "filesystem");
        }
    }

    // Set permissions if requested
    if (permissions != 0) {
        std::error_code ec;
        std::filesystem::permissions(path, 
                                     static_cast<std::filesystem::perms>(permissions),
                                     std::filesystem::perm_options::replace, ec);
        if (ec) {
            NIZAW_LOG_WARN("filesystem", "Failed to set permissions on {}: {}", 
                          path.string(), ec.message());
        }
    }

    NIZAW_LOG_INFO("filesystem", "Wrote {} bytes to '{}'", 
                   content.size(), path.string());
    core::AuditLogger::instance().log("filesystem", "write_file", 
                                      path.string(), true,
                                      std::string(std::format("bytes={}", content.size())));
    return {};
}

Result<std::string> read_file(const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        if (std::filesystem::exists(path)) {
            return Error::from_errno(errno, ErrorCode::IoError, "filesystem",
                                    "Failed to open file for reading");
        }
        return Error(ErrorCode::NotFound, "File does not exist", "filesystem");
    }

    std::string content;
    content.reserve(static_cast<size_t>(std::filesystem::file_size(path)));
    
    content.assign(std::istreambuf_iterator<char>(file),
                   std::istreambuf_iterator<char>());
    
    return content;
}

Result<void> truncate(const std::filesystem::path& path,
                      std::uintmax_t size,
                      const core::WriteOptions& options) {
    if (options.dry_run) {
        NIZAW_LOG_INFO("filesystem", "Would truncate '{}' to {} bytes", 
                      path.string(), size);
        return {};
    }

    if (options.confirm_prompt) {
        NIZAW_LOG_WARN("filesystem", "Confirmation required: {}", *options.confirm_prompt);
        return Error(ErrorCode::ConfirmationRequired, 
                     "Confirmation required for truncate", "filesystem");
    }

    std::error_code ec;
    std::filesystem::resize_file(path, size, ec);
    if (ec) {
        return Error::from_errno(ec.value(), ErrorCode::IoError, "filesystem",
                                "resize_file() failed");
    }

    NIZAW_LOG_INFO("filesystem", "Truncated '{}' to {} bytes", path.string(), size);
    core::AuditLogger::instance().log("filesystem", "truncate", path.string(), true,
                                      std::string(std::format("size={}", size)));
    return {};
}

}  // namespace nizaw::filesystem
