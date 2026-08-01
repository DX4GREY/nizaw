#include "nizaw/security.hpp"

#include <array>
#include <cerrno>
#include <fstream>
#include <sstream>
#include <string>
#include <string_view>
#include <unistd.h>

namespace nizaw::security {
namespace {

constexpr std::array<std::string_view, 39> capability_names = {
    "CAP_CHOWN",              // 0
    "CAP_DAC_OVERRIDE",       // 1
    "CAP_DAC_READ_SEARCH",    // 2
    "CAP_FOWNER",             // 3
    "CAP_FSETID",             // 4
    "CAP_KILL",               // 5
    "CAP_SETGID",             // 6
    "CAP_SETUID",             // 7
    "CAP_SETPCAP",            // 8
    "CAP_LINUX_IMMUTABLE",    // 9
    "CAP_NET_BIND_SERVICE",   // 10
    "CAP_NET_BROADCAST",      // 11
    "CAP_NET_ADMIN",          // 12
    "CAP_NET_RAW",            // 13
    "CAP_IPC_LOCK",           // 14
    "CAP_IPC_OWNER",          // 15
    "CAP_SYS_MODULE",         // 16
    "CAP_SYS_RAWIO",          // 17
    "CAP_SYS_CHROOT",         // 18
    "CAP_SYS_PTRACE",         // 19
    "CAP_SYS_PACCT",          // 20
    "CAP_SYS_ADMIN",          // 21
    "CAP_SYS_BOOT",           // 22
    "CAP_SYS_NICE",           // 23
    "CAP_SYS_RESOURCE",       // 24
    "CAP_SYS_TIME",           // 25
    "CAP_SYS_TTY_CONFIG",     // 26
    "CAP_MKNOD",              // 27
    "CAP_LEASE",              // 28
    "CAP_AUDIT_WRITE",        // 29
    "CAP_AUDIT_CONTROL",      // 30
    "CAP_SETFCAP",            // 31
    "CAP_MAC_OVERRIDE",       // 32
    "CAP_MAC_ADMIN",          // 33
    "CAP_SYSLOG",             // 34
    "CAP_WAKE_ALARM",         // 35
    "CAP_BLOCK_SUSPEND",      // 36
    "CAP_AUDIT_READ",         // 37
    "CAP_PERFMON",            // 38
};

std::optional<unsigned long long> parse_cap_mask(std::string_view value) {
    try {
        return std::stoull(std::string(value), nullptr, 16);
    } catch (const std::exception&) {
        return std::nullopt;
    }
}

std::vector<std::string> names_from_mask(unsigned long long mask) {
    std::vector<std::string> names;
    for (std::size_t index = 0; index < capability_names.size(); ++index) {
        if (mask & (1ULL << index)) {
            names.emplace_back(capability_names[index]);
        }
    }
    return names;
}

}  // namespace

Result<Identity> identity() {
    Identity info;
    info.real_uid = getuid();
    info.effective_uid = geteuid();
    info.real_gid = getgid();
    info.effective_gid = getegid();
    info.is_root = (info.effective_uid == 0);

    int group_count = getgroups(0, nullptr);
    if (group_count < 0) {
        return Error::from_errno(errno, ErrorCode::IoError, "security", "getgroups() failed");
    }

    if (group_count > 0) {
        info.groups.resize(static_cast<std::size_t>(group_count));
        if (getgroups(group_count, info.groups.data()) < 0) {
            return Error::from_errno(errno, ErrorCode::IoError, "security", "getgroups() failed");
        }
    }

    return info;
}

Result<std::vector<std::string>> capabilities() {
    std::ifstream status_file("/proc/self/status");
    if (!status_file) {
        return Error::from_errno(errno, ErrorCode::IoError, "security", "failed to open /proc/self/status");
    }

    std::string line;
    while (std::getline(status_file, line)) {
        if (line.rfind("CapEff:", 0) != 0) {
            continue;
        }

        const auto field_start = line.find_first_not_of(" \t", line.find(':') + 1);
        if (field_start == std::string::npos) {
            break;
        }

        const std::string value = line.substr(field_start);
        const auto mask = parse_cap_mask(value);
        if (!mask) {
            return Error(ErrorCode::ParseError, "Failed to parse CapEff value", "security");
        }

        return names_from_mask(*mask);
    }

    return Error(ErrorCode::NotFound, "CapEff field not found in /proc/self/status", "security");
}

}  // namespace nizaw::security
