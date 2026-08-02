#include "nizaw/system.hpp"

#include "nizaw/core/error.hpp"

#include <cerrno>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <filesystem>
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

std::optional<std::string> parse_cpu_field(std::string_view line, std::string_view key) {
    const auto colon_pos = line.find(':');
    if (colon_pos == std::string::npos) {
        return std::nullopt;
    }
    
    const auto line_key = trim(std::string(line.substr(0, colon_pos)));
    if (line_key != key) {
        return std::nullopt;
    }
    
    const auto value_start = colon_pos + 1;
    const auto value = line.substr(value_start);
    return trim(std::string(value));
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

Result<std::vector<CpuInfo>> cpu_info() {
    std::vector<CpuInfo> cpus;
    std::ifstream cpuinfo_file("/proc/cpuinfo");
    if (!cpuinfo_file) {
        return Error(ErrorCode::NotFound, "/proc/cpuinfo is unavailable", "system");
    }

    CpuInfo current{};
    std::string line;
    while (std::getline(cpuinfo_file, line)) {
        if (line.empty()) {
            if (!current.model_name.empty()) {
                cpus.push_back(current);
                current = CpuInfo{};
            }
            continue;
        }

        if (const auto model = parse_cpu_field(line, "model name")) {
            current.model_name = std::string(*model);
        }
        if (const auto vendor = parse_cpu_field(line, "vendor_id")) {
            current.vendor_id = std::string(*vendor);
        }
        if (const auto freq = parse_cpu_field(line, "cpu MHz")) {
            current.frequency_mhz = static_cast<std::size_t>(std::stod(std::string(*freq)));
        }
        if (const auto cache = parse_cpu_field(line, "cache size")) {
            const auto cache_str = trim(std::string(*cache));
            const auto unit_pos = cache_str.find(" KB");
            if (unit_pos != std::string::npos) {
                current.cache_size_kb = static_cast<std::size_t>(std::stoul(cache_str.substr(0, unit_pos)));
            }
        }
        if (const auto cores = parse_cpu_field(line, "cpu cores")) {
            current.core_count = static_cast<std::size_t>(std::stoul(std::string(*cores)));
        }
    }

    if (!current.model_name.empty()) {
        cpus.push_back(current);
    }

    return cpus;
}

Result<LoadAverage> load_average() {
    std::ifstream loadavg_file("/proc/loadavg");
    if (!loadavg_file) {
        return Error(ErrorCode::NotFound, "/proc/loadavg is unavailable", "system");
    }

    LoadAverage load{};
    std::string line;
    if (std::getline(loadavg_file, line)) {
        std::istringstream stream(line);
        stream >> load.one_min >> load.five_min >> load.fifteen_min;
    }

    return load;
}

Result<MemoryInfo> memory_info() {
    std::ifstream meminfo_file("/proc/meminfo");
    if (!meminfo_file) {
        return Error(ErrorCode::NotFound, "/proc/meminfo is unavailable", "system");
    }

    MemoryInfo mem{};
    std::string line;
    while (std::getline(meminfo_file, line)) {
        const auto separator = line.find(':');
        if (separator == std::string::npos) {
            continue;
        }

        const std::string key = trim(line.substr(0, separator));
        const std::string value_str = trim(line.substr(separator + 1));
        const auto unit_pos = value_str.find(" kB");
        std::uint64_t value_kb = 0;
        if (unit_pos != std::string::npos) {
            try {
                value_kb = static_cast<std::uint64_t>(std::stoull(value_str.substr(0, unit_pos)));
            } catch (const std::exception&) {
                continue;
            }
        }

        if (key == "MemTotal") {
            mem.total_kb = value_kb;
        } else if (key == "MemFree") {
            mem.free_kb = value_kb;
        } else if (key == "MemAvailable") {
            mem.available_kb = value_kb;
        } else if (key == "Buffers") {
            mem.buffers_kb = value_kb;
        } else if (key == "Cached") {
            mem.cached_kb = value_kb;
        } else if (key == "Shmem") {
            mem.shared_kb = value_kb;
        }
    }

    return mem;
}

Result<SwapInfo> swap_info() {
    std::ifstream meminfo_file("/proc/meminfo");
    if (!meminfo_file) {
        return Error(ErrorCode::NotFound, "/proc/meminfo is unavailable", "system");
    }

    SwapInfo swap{};
    std::string line;
    while (std::getline(meminfo_file, line)) {
        const auto separator = line.find(':');
        if (separator == std::string::npos) {
            continue;
        }

        const std::string key = trim(line.substr(0, separator));
        const std::string value_str = trim(line.substr(separator + 1));
        const auto unit_pos = value_str.find(" kB");
        std::uint64_t value_kb = 0;
        if (unit_pos != std::string::npos) {
            try {
                value_kb = static_cast<std::uint64_t>(std::stoull(value_str.substr(0, unit_pos)));
            } catch (const std::exception&) {
                continue;
            }
        }

        if (key == "SwapTotal") {
            swap.total_kb = value_kb;
        } else if (key == "SwapFree") {
            swap.free_kb = value_kb;
        }
    }

    swap.used_kb = swap.total_kb - swap.free_kb;
    return swap;
}

Result<std::vector<std::string>> kernel_modules() {
    std::ifstream modules_file("/proc/modules");
    if (!modules_file) {
        return Error(ErrorCode::NotFound, "/proc/modules is unavailable", "system");
    }

    std::vector<std::string> modules;
    std::string line;
    while (std::getline(modules_file, line)) {
        const auto separator = line.find(' ');
        if (separator != std::string::npos) {
            modules.push_back(trim(line.substr(0, separator)));
        }
    }

    return modules;
}

Result<std::vector<HwmonData>> hwmon() {
    const std::filesystem::path hwmon_path("/sys/class/hwmon");
    if (!std::filesystem::exists(hwmon_path)) {
        return Error(ErrorCode::NotFound, "/sys/class/hwmon is unavailable", "system");
    }

    std::vector<HwmonData> sensors;
    std::error_code ec;

    for (const auto& entry : std::filesystem::directory_iterator(hwmon_path, ec)) {
        if (ec) {
            break;
        }
        if (!entry.is_directory(ec)) {
            continue;
        }

        const std::string name = entry.path().filename().string();
        const std::string name_path = entry.path().string();
        std::string sensor_name;
        std::string sensor_type;

        // Try to get sensor name
        std::ifstream name_file(name_path + "/name");
        if (name_file) {
            std::getline(name_file, sensor_name);
        }

        // Check for temperature
        for (const auto& temp_entry : std::filesystem::directory_iterator(name_path, ec)) {
            if (ec || !temp_entry.is_regular_file(ec)) {
                continue;
            }
            const std::string filename = temp_entry.path().filename().string();
            if (filename.starts_with("temp") && filename.ends_with("_input")) {
                std::ifstream temp_file(temp_entry.path());
                if (temp_file) {
                    std::string value;
                    std::getline(temp_file, value);
                    if (!value.empty()) {
                        HwmonData data;
                        data.sensor_name = sensor_name.empty() ? name : sensor_name;
                        data.sensor_type = "temperature";
                        data.temperature_celsius = std::stod(value) / 1000.0;
                        sensors.push_back(data);
                    }
                }
            } else if (filename.starts_with("fan") && filename.ends_with("_input")) {
                std::ifstream fan_file(temp_entry.path());
                if (fan_file) {
                    std::string value;
                    std::getline(fan_file, value);
                    if (!value.empty()) {
                        HwmonData data;
                        data.sensor_name = sensor_name.empty() ? name : sensor_name;
                        data.sensor_type = "fan";
                        data.fan_speed_rpm = std::stod(value);
                        sensors.push_back(data);
                    }
                }
            }
        }
    }

    return sensors;
}

}  // namespace nizaw::system
