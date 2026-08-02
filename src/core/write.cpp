#include "nizaw/core/write.hpp"

#include "nizaw/core/log.hpp"

#include <cerrno>
#include <cstring>
#include <cstdint>
#include <format>
#include <fstream>
#include <linux/capability.h>
#include <sys/capability.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <unistd.h>

namespace nizaw::core {

bool CapabilitySet::check(int capability) const noexcept {
    // If we've never loaded capabilities, try to read them
    if (!capabilities_loaded_) {
        load_capabilities();
    }

    // Check if the capability is in our set
    for (int cap : capabilities_) {
        if (cap == capability) {
            return true;
        }
    }
    return false;
}

void CapabilitySet::load_capabilities() const noexcept {
    // If already loaded, don't load again
    if (capabilities_loaded_) {
        return;
    }
    capabilities_loaded_ = true;

    // Get real and effective UIDs
    uid_ = ::getuid();
    euid_ = ::geteuid();

    // Try to read the process capabilities from /proc
    // This is a best-effort approach
    const char* path = "/proc/self/status";

    // Read the contents of /proc/self/status to find capabilities
    std::ifstream file(path);
    if (!file.is_open()) {
        // If we can't read the file, assume no special capabilities
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        if (line.find("CapEff") != std::string::npos) {
            // Parse the capability effective set
            // Format: CapEff:    0000003fffffffff
            const char* hex_str = line.c_str() + 8;
            while (*hex_str == ' ' || *hex_str == '\t') {
                ++hex_str;
            }
            char* end_ptr = nullptr;
            unsigned long long cap_eff = std::strtoull(hex_str, &end_ptr, 16);
            if (end_ptr == hex_str) {
                continue;
            }
            capabilities_.reserve(64);
            for (int cap = 0; cap < 64; ++cap) {
                if (cap_eff & (1ULL << cap)) {
                    capabilities_.push_back(cap);
                }
            }
            break;
        }
    }
}

bool CapabilitySet::has_admin() const noexcept {
    return check(CAP_SYS_ADMIN);
}

bool CapabilitySet::has_network_admin() const noexcept {
    return check(CAP_NET_ADMIN);
}

bool CapabilitySet::has_dac_override() const noexcept {
    return check(CAP_DAC_OVERRIDE);
}

bool CapabilitySet::has_setuid() const noexcept {
    return check(CAP_SETUID);
}

bool CapabilitySet::has_setgid() const noexcept {
    return check(CAP_SETGID);
}

bool CapabilitySet::has_kill() const noexcept {
    return check(CAP_KILL);
}

bool CapabilitySet::has_sys_ptrace() const noexcept {
    return check(CAP_SYS_PTRACE);
}

bool CapabilitySet::has_sys_time() const noexcept {
    return check(CAP_SYS_TIME);
}

bool CapabilitySet::has_setpcap() const noexcept {
    return check(CAP_SETPCAP);
}

bool CapabilitySet::has_setfcaps() const noexcept {
    return check(CAP_SETFCAP);
}

bool CapabilitySet::is_root() const noexcept {
    if (!capabilities_loaded_) {
        load_capabilities();
    }
    return uid_ == 0;
}

uid_t CapabilitySet::real_uid() const noexcept {
    if (!capabilities_loaded_) {
        load_capabilities();
    }
    return uid_;
}

uid_t CapabilitySet::effective_uid() const noexcept {
    if (!capabilities_loaded_) {
        load_capabilities();
    }
    return euid_;
}

CapabilitySet CapabilitySet::from_current() noexcept {
    CapabilitySet caps;
    caps.load_capabilities();
    return caps;
}

CapabilitySet current_capabilities() noexcept {
    return CapabilitySet::from_current();
}

AuditLogger& AuditLogger::instance() noexcept {
    static AuditLogger logger;
    return logger;
}

void AuditLogger::log(const std::string& module,
                      const std::string& operation,
                      const std::string& target,
                      bool success,
                      const std::string& details) {
    if (level_ == 0) {
        return;
    }

    const char* status = success ? "SUCCESS" : "FAILED";
    // Build message without std::format for now
    std::string message = "[AUDIT] " + module + "::" + operation + 
                          " on '" + target + "' - " + status;
    if (!details.empty()) {
        message += " - " + details;
    }

    // Use the core logging system via Logger instance
    Logger::instance().write(LogLevel::Info, "audit", message);
}

void AuditLogger::set_level(int level) noexcept {
    level_ = level;
}

}  // namespace nizaw::core