#include "nizaw/system.hpp"

#include "nizaw/core/error.hpp"

#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <sys/utsname.h>
#include <unistd.h>

namespace nizaw::system {
namespace {

std::string trim(std::string value) {
    const auto start = value.find_first_not_of(" \t\n\r");
    if (start == std::string::npos) {
        return {};
    }
    const auto end = value.find_last_not_of(" \t\n\r");
    return value.substr(start, end - start + 1);
}

std::string format_uptime(double seconds) {
    const auto total_seconds = static_cast<unsigned long long>(std::llround(seconds));
    const auto days = total_seconds / 86400ULL;
    const auto hours = (total_seconds % 86400ULL) / 3600ULL;
    const auto minutes = (total_seconds % 3600ULL) / 60ULL;
    const auto secs = total_seconds % 60ULL;

    std::ostringstream out;
    if (days > 0) {
        out << days << "d ";
    }
    if (hours > 0) {
        out << hours << "h ";
    }
    if (minutes > 0) {
        out << minutes << "m ";
    }
    out << secs << "s";
    return out.str();
}

std::string format_boot_time(long long btime) {
    const auto time_value = static_cast<std::time_t>(btime);
    std::tm tm_value{};
#ifdef _WIN32
    localtime_s(&tm_value, &time_value);
#else
    localtime_r(&time_value, &tm_value);
#endif
    std::ostringstream out;
    out << std::put_time(&tm_value, "%Y-%m-%d %H:%M:%S");
    return out.str();
}

}  // namespace

Result<SystemInfo> info() {
    SystemInfo info{};

    struct utsname kernel_info {};
    if (uname(&kernel_info) != 0) {
        return Error::from_errno(errno, ErrorCode::IoError, "system",
                                 "uname() failed");
    }

    info.kernel_name = kernel_info.sysname;
    info.kernel_release = kernel_info.release;
    info.kernel_version = kernel_info.version;
    info.architecture = kernel_info.machine;

    char hostname[256]{};
    if (gethostname(hostname, sizeof(hostname) - 1) != 0) {
        return Error::from_errno(errno, ErrorCode::IoError, "system",
                                 "gethostname() failed");
    }
    info.hostname = hostname;

    const long page_size = sysconf(_SC_PAGESIZE);
    if (page_size < 0) {
        return Error::from_errno(errno, ErrorCode::IoError, "system",
                                 "sysconf(_SC_PAGESIZE) failed");
    }
    info.page_size = static_cast<std::size_t>(page_size);

    const long cpu_count = sysconf(_SC_NPROCESSORS_ONLN);
    if (cpu_count < 0) {
        return Error::from_errno(errno, ErrorCode::IoError, "system",
                                 "sysconf(_SC_NPROCESSORS_ONLN) failed");
    }
    info.cpu_count = static_cast<std::size_t>(cpu_count);

    std::ifstream uptime_file("/proc/uptime");
    if (!uptime_file) {
        return Error(ErrorCode::NotFound, "/proc/uptime is unavailable", "system");
    }
    std::string uptime_line;
    std::getline(uptime_file, uptime_line);
    const auto whitespace = uptime_line.find(' ');
    if (whitespace == std::string::npos) {
        return Error(ErrorCode::ParseError, "Unable to parse /proc/uptime", "system");
    }
    const auto uptime_seconds = std::stod(trim(uptime_line.substr(0, whitespace)));
    info.uptime = format_uptime(uptime_seconds);

    std::ifstream stat_file("/proc/stat");
    if (stat_file) {
        std::string line;
        while (std::getline(stat_file, line)) {
            if (line.rfind("btime ", 0) == 0) {
                const auto btime = std::stoll(trim(line.substr(6)));
                info.boot_time = format_boot_time(btime);
                break;
            }
        }
    }

    return info;
}

}  // namespace nizaw::system
