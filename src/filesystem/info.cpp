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

}  // namespace nizaw::filesystem
