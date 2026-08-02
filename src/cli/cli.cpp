#include "nizaw/cli.hpp"

#include "nizaw/core/version.hpp"
#include "nizaw/filesystem.hpp"
#include "nizaw/network.hpp"
#include "nizaw/plugin.hpp"
#include "nizaw/process.hpp"
#include "nizaw/security.hpp"
#include "nizaw/service.hpp"
#include "nizaw/storage.hpp"
#include "nizaw/system.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <iostream>
#include <numeric>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace nizaw::cli {
namespace {

struct Options {
    bool help = false;
    bool version = false;
    bool json = false;
    bool verbose = false;
    bool quiet = false;
    bool no_color = false;
    std::vector<std::string> args;
};

std::string json_escape(std::string_view value) {
    std::string escaped;
    escaped.reserve(value.size());
    for (char ch : value) {
        switch (ch) {
            case '"': escaped += "\\\""; break;
            case '\\': escaped += "\\\\"; break;
            case '\b': escaped += "\\b"; break;
            case '\f': escaped += "\\f"; break;
            case '\n': escaped += "\\n"; break;
            case '\r': escaped += "\\r"; break;
            case '\t': escaped += "\\t"; break;
            default:
                if (static_cast<unsigned char>(ch) < 0x20) {
                    char buffer[7];
                    std::snprintf(buffer, sizeof(buffer), "\\u%04x", static_cast<unsigned char>(ch));
                    escaped += buffer;
                } else {
                    escaped += ch;
                }
        }
    }
    return escaped;
}

std::string json_string(std::string_view value) {
    return std::string("\"") + json_escape(value) + "\"";
}

std::string join_json_array(const std::vector<std::string>& items) {
    std::string result = "[";
    for (std::size_t i = 0; i < items.size(); ++i) {
        if (i > 0) {
            result += ",";
        }
        result += items[i];
    }
    result += "]";
    return result;
}

void print_row(std::string_view label, std::string_view value) {
    std::cout << label << ": " << value << '\n';
}

void print_row(std::string_view label, bool value) {
    print_row(label, std::string_view(value ? "true" : "false"));
}

void print_error(const Error& error, bool json) {
    if (json) {
        std::cout << "{\"error\":{"
                  << "\"code\":" << json_string(to_string(error.code()))
                  << ",\"message\":" << json_string(error.message())
                  << ",\"source\":" << json_string(error.source())
                  << "}}\n";
    } else {
        std::cerr << "Error [" << to_string(error.code()) << "] from " << error.source() << ": " << error.message() << '\n';
    }
}

void print_help() {
    std::cout << "Usage: nizaw [--help] [--version] [--json] [--verbose] [--quiet] [--no-color] <command> [args...]\n"
              << "Commands:\n"
              << "  system info\n"
              << "  system cpu\n"
              << "  system memory\n"
              << "  system load\n"
              << "  system modules\n"
              << "  system hwmon\n"
              << "  process list\n"
              << "  process inspect <PID>\n"
              << "  process environment <PID>\n"
              << "  process signal <PID> <SIGNAL>\n"
              << "  process terminate <PID> [--force]\n"
              << "  process suspend <PID>\n"
              << "  process resume <PID>\n"
              << "  process nice <PID> <VALUE>\n"
              << "  fs usage <PATH>\n"
              << "  fs info <PATH>\n"
              << "  fs mounts\n"
              << "  fs mkdir <PATH> [--recursive] [--mode <OCTAL>]\n"
              << "  fs rm <PATH> [--recursive]\n"
              << "  fs cp <FROM> <TO> [--recursive]\n"
              << "  fs mv <FROM> <TO>\n"
              << "  fs write <PATH> <CONTENT>\n"
              << "  fs chmod <PATH> <MODE>\n"
              << "  fs chown <PATH> <UID>:<GID>\n"
              << "  storage list\n"
              << "  storage info <DEVICE>\n"
              << "  storage iostat <DEVICE>\n"
              << "  network interfaces\n"
              << "  network info <IFACE>\n"
              << "  network connections\n"
              << "  service list\n"
              << "  service status <UNIT>\n"
              << "  service inspect <UNIT>\n"
              << "  service start <UNIT>\n"
              << "  service stop <UNIT>\n"
              << "  service restart <UNIT>\n"
              << "  service reload <UNIT>\n"
              << "  service enable <UNIT>\n"
              << "  service disable <UNIT>\n"
              << "  security identity\n"
              << "  security capabilities\n"
              << "  plugins list [DIRECTORY]\n";
}

Options parse_options(int argc, char* argv[]) {
    Options options;
    for (int index = 1; index < argc; ++index) {
        const std::string arg = argv[index];
        if (arg == "--help" || arg == "-h") {
            options.help = true;
        } else if (arg == "--version") {
            options.version = true;
        } else if (arg == "--json" || arg == "-j") {
            options.json = true;
        } else if (arg == "--verbose" || arg == "-v") {
            options.verbose = true;
        } else if (arg == "--quiet") {
            options.quiet = true;
        } else if (arg == "--no-color") {
            options.no_color = true;
        } else if (!arg.empty() && arg[0] == '-') {
            // Pass through unknown flags to command handlers
            options.args.emplace_back(arg);
        } else {
            options.args.emplace_back(arg);
        }
    }
    return options;
}

int run_system_info(const Options& options) {
    const auto result = system::info();
    if (!result) {
        print_error(result.error(), options.json);
        return 1;
    }
    const auto& info = result.value();
    if (options.json) {
        std::cout << "{"
                  << "\"hostname\":" << json_string(info.hostname)
                  << ",\"kernel_name\":" << json_string(info.kernel_name)
                  << ",\"kernel_release\":" << json_string(info.kernel_release)
                  << ",\"kernel_version\":" << json_string(info.kernel_version)
                  << ",\"architecture\":" << json_string(info.architecture)
                  << ",\"uptime\":" << json_string(info.uptime)
                  << ",\"boot_time\":" << json_string(info.boot_time)
                  << ",\"page_size\":" << info.page_size
                  << ",\"cpu_count\":" << info.cpu_count
                  << "}\n";
    } else {
        print_row("Hostname", info.hostname);
        print_row("Kernel", info.kernel_name);
        print_row("Kernel release", info.kernel_release);
        print_row("Kernel version", info.kernel_version);
        print_row("Architecture", info.architecture);
        print_row("Uptime", info.uptime);
        print_row("Boot time", info.boot_time);
        print_row("Page size", std::to_string(info.page_size));
        print_row("CPU count", std::to_string(info.cpu_count));
    }
    return 0;
}

int run_system_cpu(const Options& options) {
    const auto result = system::cpu_info();
    if (!result) {
        print_error(result.error(), options.json);
        return 1;
    }
    const auto& cpus = result.value();
    if (options.json) {
        std::vector<std::string> items;
        for (const auto& cpu : cpus) {
            std::string item = "{"
                "\"model_name\":" + json_string(cpu.model_name) +
                ",\"vendor_id\":" + json_string(cpu.vendor_id) +
                ",\"frequency_mhz\":" + std::to_string(cpu.frequency_mhz) +
                ",\"cache_size_kb\":" + std::to_string(cpu.cache_size_kb) +
                ",\"core_count\":" + std::to_string(cpu.core_count) +
                "}";
            items.push_back(item);
        }
        std::cout << join_json_array(items) << "\n";
    } else {
        for (const auto& cpu : cpus) {
            std::cout << "Model: " << cpu.model_name << "\n";
            std::cout << "  Vendor: " << cpu.vendor_id << "\n";
            std::cout << "  Frequency: " << cpu.frequency_mhz << " MHz\n";
            std::cout << "  Cache: " << cpu.cache_size_kb << " KB\n";
            std::cout << "  Cores: " << cpu.core_count << "\n\n";
        }
    }
    return 0;
}

int run_system_memory(const Options& options) {
    const auto mem_result = system::memory_info();
    if (!mem_result) {
        print_error(mem_result.error(), options.json);
        return 1;
    }
    const auto& mem = mem_result.value();

    const auto swap_result = system::swap_info();
    if (!swap_result) {
        print_error(swap_result.error(), options.json);
        return 1;
    }
    const auto& swap = swap_result.value();

    if (options.json) {
        std::cout << "{"
                  << "\"memory\":{"
                  << "\"total_kb\":" << mem.total_kb
                  << ",\"free_kb\":" << mem.free_kb
                  << ",\"available_kb\":" << mem.available_kb
                  << ",\"buffers_kb\":" << mem.buffers_kb
                  << ",\"cached_kb\":" << mem.cached_kb
                  << ",\"shared_kb\":" << mem.shared_kb
                  << "},"
                  << "\"swap\":{"
                  << "\"total_kb\":" << swap.total_kb
                  << ",\"used_kb\":" << swap.used_kb
                  << ",\"free_kb\":" << swap.free_kb
                  << "}"
                  << "}\n";
    } else {
        print_row("Memory total", std::to_string(mem.total_kb) + " KB");
        print_row("Memory free", std::to_string(mem.free_kb) + " KB");
        print_row("Memory available", std::to_string(mem.available_kb) + " KB");
        print_row("Buffers", std::to_string(mem.buffers_kb) + " KB");
        print_row("Cached", std::to_string(mem.cached_kb) + " KB");
        print_row("Shared", std::to_string(mem.shared_kb) + " KB");
        print_row("Swap total", std::to_string(swap.total_kb) + " KB");
        print_row("Swap used", std::to_string(swap.used_kb) + " KB");
        print_row("Swap free", std::to_string(swap.free_kb) + " KB");
    }
    return 0;
}

int run_system_load(const Options& options) {
    const auto result = system::load_average();
    if (!result) {
        print_error(result.error(), options.json);
        return 1;
    }
    const auto& load = result.value();
    if (options.json) {
        std::cout << "{"
                  << "\"one_min\":" << load.one_min
                  << ",\"five_min\":" << load.five_min
                  << ",\"fifteen_min\":" << load.fifteen_min
                  << "}\n";
    } else {
        print_row("Load average (1 min)", std::to_string(load.one_min));
        print_row("Load average (5 min)", std::to_string(load.five_min));
        print_row("Load average (15 min)", std::to_string(load.fifteen_min));
    }
    return 0;
}

int run_system_modules(const Options& options) {
    const auto result = system::kernel_modules();
    if (!result) {
        print_error(result.error(), options.json);
        return 1;
    }
    const auto& modules = result.value();
    if (options.json) {
        std::vector<std::string> items;
        for (const auto& module : modules) {
            items.push_back(json_string(module));
        }
        std::cout << join_json_array(items) << "\n";
    } else {
        for (const auto& module : modules) {
            std::cout << module << "\n";
        }
    }
    return 0;
}

int run_system_hwmon(const Options& options) {
    const auto result = system::hwmon();
    if (!result) {
        print_error(result.error(), options.json);
        return 1;
    }
    const auto& sensors = result.value();
    if (options.json) {
        std::vector<std::string> items;
        for (const auto& sensor : sensors) {
            std::string item = "{"
                "\"sensor_name\":" + json_string(sensor.sensor_name) +
                ",\"sensor_type\":" + json_string(sensor.sensor_type) +
                ",\"temperature_celsius\":" + std::to_string(sensor.temperature_celsius) +
                ",\"fan_speed_rpm\":" + std::to_string(sensor.fan_speed_rpm) +
                ",\"voltage\":" + std::to_string(sensor.voltage) +
                ",\"power_consumption\":" + std::to_string(sensor.power_consumption) +
                "}";
            items.push_back(item);
        }
        std::cout << join_json_array(items) << "\n";
    } else {
        for (const auto& sensor : sensors) {
            std::cout << sensor.sensor_name << " (" << sensor.sensor_type << ")\n";
            if (sensor.sensor_type == "temperature") {
                std::cout << "  Temperature: " << sensor.temperature_celsius << "°C\n";
            } else if (sensor.sensor_type == "fan") {
                std::cout << "  Fan speed: " << sensor.fan_speed_rpm << " RPM\n";
            }
        }
    }
    return 0;
}

int run_process_list(const Options& options) {
    const auto result = process::list();
    if (!result) {
        print_error(result.error(), options.json);
        return 1;
    }
    if (options.json) {
        std::vector<std::string> items;
        for (const auto& process : result.value()) {
            std::string item = "{"
                "\"pid\":" + std::to_string(process.pid) +
                ",\"ppid\":" + std::to_string(process.ppid) +
                ",\"uid\":" + std::to_string(process.uid) +
                ",\"gid\":" + std::to_string(process.gid) +
                ",\"name\":" + json_string(process.name) +
                ",\"state\":" + json_string(process.state) +
                ",\"command\":" + json_string(process.command) +
                ",\"executable\":" + json_string(process.executable) +
                ",\"arguments\":" + json_string(process.arguments) +
                ",\"threads\":" + std::to_string(process.threads) +
                ",\"memory_kb\":" + std::to_string(process.memory_kb) +
                ",\"cpu_time_seconds\":" + std::to_string(process.cpu_time_seconds) +
                ",\"start_time\":" + json_string(process.start_time) +
                "}";
            items.push_back(item);
        }
        std::cout << join_json_array(items) << "\n";
    } else {
        for (const auto& process : result.value()) {
            std::cout << process.pid << "\t" << process.name << "\t" << process.state << '\n';
        }
    }
    return 0;
}

int run_process_inspect(const Options& options, std::string_view pid_text) {
    try {
        const auto pid = static_cast<pid_t>(std::stoll(std::string(pid_text)));
        const auto result = process::inspect(pid);
        if (!result) {
            print_error(result.error(), options.json);
            return 1;
        }
        const auto& info = result.value();
        if (options.json) {
            std::cout << "{"
                      << "\"pid\":" << info.pid
                      << ",\"ppid\":" << info.ppid
                      << ",\"uid\":" << info.uid
                      << ",\"gid\":" << info.gid
                      << ",\"name\":" << json_string(info.name)
                      << ",\"state\":" << json_string(info.state)
                      << ",\"command\":" << json_string(info.command)
                      << ",\"executable\":" << json_string(info.executable)
                      << ",\"arguments\":" << json_string(info.arguments)
                      << ",\"threads\":" << info.threads
                      << ",\"memory_kb\":" << info.memory_kb
                      << ",\"cpu_time_seconds\":" << info.cpu_time_seconds
                      << ",\"start_time\":" << json_string(info.start_time)
                      << "}\n";
        } else {
            print_row("PID", std::to_string(info.pid));
            print_row("PPID", std::to_string(info.ppid));
            print_row("UID", std::to_string(info.uid));
            print_row("GID", std::to_string(info.gid));
            print_row("Name", info.name);
            print_row("State", info.state);
            print_row("Command", info.command);
            print_row("Executable", info.executable);
            print_row("Arguments", info.arguments);
            print_row("Threads", std::to_string(info.threads));
            print_row("Memory KB", std::to_string(info.memory_kb));
            print_row("CPU time seconds", std::to_string(info.cpu_time_seconds));
            print_row("Start time", info.start_time);
        }
        return 0;
    } catch (const std::exception&) {
        std::cerr << "Invalid PID: " << pid_text << '\n';
        return 2;
    }
}

int run_filesystem_usage(const Options& options, std::string_view path) {
    const auto result = filesystem::usage(std::filesystem::path(path));
    if (!result) {
        print_error(result.error(), options.json);
        return 1;
    }
    const auto& usage = result.value();
    if (options.json) {
        std::cout << "{"
                  << "\"total_bytes\":" << usage.total_bytes
                  << ",\"free_bytes\":" << usage.free_bytes
                  << ",\"available_bytes\":" << usage.available_bytes
                  << ",\"used_bytes\":" << usage.used_bytes
                  << "}\n";
    } else {
        print_row("Total bytes", std::to_string(usage.total_bytes));
        print_row("Free bytes", std::to_string(usage.free_bytes));
        print_row("Available bytes", std::to_string(usage.available_bytes));
        print_row("Used bytes", std::to_string(usage.used_bytes));
    }
    return 0;
}

int run_filesystem_info(const Options& options, std::string_view path) {
    const auto result = filesystem::info(std::filesystem::path(path));
    if (!result) {
        print_error(result.error(), options.json);
        return 1;
    }
    const auto& info = result.value();
    if (options.json) {
        std::cout << "{"
                  << "\"path\":" << json_string(info.path)
                  << ",\"type\":" << json_string(info.type)
                  << ",\"permissions\":" << json_string(info.permissions)
                  << ",\"owner\":" << json_string(info.owner)
                  << ",\"group\":" << json_string(info.group)
                  << ",\"size_bytes\":" << info.size_bytes
                  << ",\"inode\":" << info.inode
                  << ",\"exists\":" << (info.exists ? "true" : "false")
                  << ",\"is_symlink\":" << (info.is_symlink ? "true" : "false")
                  << ",\"mount_point\":" << json_string(info.mount_point)
                  << "}\n";
    } else {
        print_row("Path", info.path);
        print_row("Type", info.type);
        print_row("Permissions", info.permissions);
        print_row("Owner", info.owner);
        print_row("Group", info.group);
        print_row("Size bytes", std::to_string(info.size_bytes));
        print_row("Inode", std::to_string(info.inode));
        print_row("Exists", info.exists);
        print_row("Is symlink", info.is_symlink);
        print_row("Mount point", info.mount_point);
    }
    return 0;
}

int run_storage_list(const Options& options) {
    const auto result = storage::enumerate();
    if (!result) {
        print_error(result.error(), options.json);
        return 1;
    }
    if (options.json) {
        std::vector<std::string> items;
        for (const auto& device : result.value()) {
            std::string item = "{"
                "\"name\":" + json_string(device.name) +
                ",\"sys_path\":" + json_string(device.sys_path) +
                ",\"dev_node\":" + json_string(device.dev_node) +
                ",\"model\":" + json_string(device.model) +
                ",\"vendor\":" + json_string(device.vendor) +
                ",\"size_bytes\":" + std::to_string(device.size_bytes) +
                ",\"logical_block_size\":" + std::to_string(device.logical_block_size) +
                ",\"physical_block_size\":" + std::to_string(device.physical_block_size) +
                ",\"removable\":" + (device.removable ? "true" : "false") +
                ",\"read_only\":" + (device.read_only ? "true" : "false") +
                ",\"rotational\":" + (device.rotational ? "true" : "false") +
                ",\"type\":" + json_string(std::to_string(static_cast<int>(device.type))) +
                "}";
            items.push_back(item);
        }
        std::cout << join_json_array(items) << "\n";
    } else {
        for (const auto& device : result.value()) {
            std::cout << device.name << "\t" << device.dev_node << "\t" << device.model << "\n";
        }
    }
    return 0;
}

int run_storage_info(const Options& options, std::string_view device_name) {
    const auto result = storage::inspect(std::string(device_name));
    if (!result) {
        print_error(result.error(), options.json);
        return 1;
    }
    const auto& device = result.value();
    if (options.json) {
        std::cout << "{"
                  << "\"name\":" << json_string(device.name)
                  << ",\"sys_path\":" << json_string(device.sys_path)
                  << ",\"dev_node\":" << json_string(device.dev_node)
                  << ",\"model\":" << json_string(device.model)
                  << ",\"vendor\":" << json_string(device.vendor)
                  << ",\"size_bytes\":" << device.size_bytes
                  << ",\"logical_block_size\":" << device.logical_block_size
                  << ",\"physical_block_size\":" << device.physical_block_size
                  << ",\"removable\":" << (device.removable ? "true" : "false")
                  << ",\"read_only\":" << (device.read_only ? "true" : "false")
                  << ",\"rotational\":" << (device.rotational ? "true" : "false")
                  << ",\"type\":" << json_string(std::to_string(static_cast<int>(device.type)))
                  << "}\n";
    } else {
        print_row("Name", device.name);
        print_row("Sys path", device.sys_path);
        print_row("Device node", device.dev_node);
        print_row("Model", device.model);
        print_row("Vendor", device.vendor);
        print_row("Size bytes", std::to_string(device.size_bytes));
        print_row("Logical block size", std::to_string(device.logical_block_size));
        print_row("Physical block size", std::to_string(device.physical_block_size));
        print_row("Removable", device.removable);
        print_row("Read only", device.read_only);
        print_row("Rotational", device.rotational);
        print_row("Type", std::to_string(static_cast<int>(device.type)));
    }
    return 0;
}

int run_network_interfaces(const Options& options) {
    const auto result = network::list();
    if (!result) {
        print_error(result.error(), options.json);
        return 1;
    }
    if (options.json) {
        std::vector<std::string> items;
        for (const auto& iface : result.value()) {
            std::vector<std::string> addresses;
            for (const auto& address : iface.addresses) {
                addresses.push_back("{"
                    "\"family\":" + json_string(address.family) +
                    ",\"address\":" + json_string(address.address) +
                    ",\"netmask\":" + json_string(address.netmask) +
                    ",\"broadcast\":" + json_string(address.broadcast) +
                    "}");
            }
            std::string item = "{"
                "\"name\":" + json_string(iface.name) +
                ",\"index\":" + std::to_string(iface.index) +
                ",\"state\":" + json_string(iface.state) +
                ",\"mac_address\":" + json_string(iface.mac_address) +
                ",\"mtu\":" + std::to_string(iface.mtu) +
                ",\"flags\":" + join_json_array(iface.flags) +
                ",\"addresses\":" + join_json_array(addresses) +
                "}";
            items.push_back(item);
        }
        std::cout << join_json_array(items) << "\n";
    } else {
        for (const auto& iface : result.value()) {
            std::cout << iface.name << "\t" << iface.state << "\t" << iface.mac_address << "\n";
        }
    }
    return 0;
}

int run_network_info(const Options& options, std::string_view interface_name) {
    const auto result = network::inspect(interface_name);
    if (!result) {
        print_error(result.error(), options.json);
        return 1;
    }
    const auto& iface = result.value();
    if (options.json) {
        std::vector<std::string> addresses;
        for (const auto& address : iface.addresses) {
            addresses.push_back("{"
                "\"family\":" + json_string(address.family) +
                ",\"address\":" + json_string(address.address) +
                ",\"netmask\":" + json_string(address.netmask) +
                ",\"broadcast\":" + json_string(address.broadcast) +
                "}");
        }
        std::cout << "{"
                  << "\"name\":" << json_string(iface.name)
                  << ",\"index\":" << iface.index
                  << ",\"state\":" << json_string(iface.state)
                  << ",\"mac_address\":" << json_string(iface.mac_address)
                  << ",\"mtu\":" << iface.mtu
                  << ",\"flags\":" << join_json_array(iface.flags)
                  << ",\"addresses\":" << join_json_array(addresses)
                  << "}\n";
    } else {
        print_row("Name", iface.name);
        print_row("Index", std::to_string(iface.index));
        print_row("State", iface.state);
        print_row("MAC address", iface.mac_address);
        print_row("MTU", std::to_string(iface.mtu));
        print_row("Flags", std::accumulate(iface.flags.begin(), iface.flags.end(), std::string(), [](std::string a, const std::string& b) {
            return a.empty() ? b : a + ", " + b;
        }));
        for (const auto& address : iface.addresses) {
            std::cout << "Address: " << address.address << " (" << address.family << ", netmask=" << address.netmask << ", broadcast=" << address.broadcast << ")\n";
        }
    }
    return 0;
}

int run_service_list(const Options& options) {
    const auto result = service::list();
    if (!result) {
        print_error(result.error(), options.json);
        return 1;
    }
    if (options.json) {
        std::vector<std::string> items;
        for (const auto& service_info : result.value()) {
            items.push_back("{"
                "\"name\":" + json_string(service_info.name) +
                ",\"description\":" + json_string(service_info.description) +
                ",\"load_state\":" + json_string(service_info.load_state) +
                ",\"active_state\":" + json_string(service_info.active_state) +
                ",\"sub_state\":" + json_string(service_info.sub_state) +
                ",\"loaded\":" + (service_info.loaded ? "true" : "false") +
                ",\"active\":" + (service_info.active ? "true" : "false") +
                ",\"enabled\":" + (service_info.enabled.has_value() ? (service_info.enabled.value() ? "true" : "false") : "null") +
                ",\"main_pid\":" + (service_info.main_pid.has_value() ? std::to_string(service_info.main_pid.value()) : std::string("null")) +
                "}");
        }
        std::cout << join_json_array(items) << "\n";
    } else {
        for (const auto& service_info : result.value()) {
            std::cout << service_info.name << "\t" << service_info.active_state << "\t" << service_info.load_state << '\n';
        }
    }
    return 0;
}

int run_service_inspect(const Options& options, std::string_view unit_name) {
    const auto result = service::inspect(unit_name);
    if (!result) {
        print_error(result.error(), options.json);
        return 1;
    }
    const auto& service_info = result.value();
    if (options.json) {
        std::cout << "{"
                  << "\"name\":" << json_string(service_info.name)
                  << ",\"description\":" << json_string(service_info.description)
                  << ",\"load_state\":" << json_string(service_info.load_state)
                  << ",\"active_state\":" << json_string(service_info.active_state)
                  << ",\"sub_state\":" << json_string(service_info.sub_state)
                  << ",\"loaded\":" << (service_info.loaded ? "true" : "false")
                  << ",\"active\":" << (service_info.active ? "true" : "false")
                  << ",\"enabled\":" << (service_info.enabled.has_value() ? (service_info.enabled.value() ? "true" : "false") : std::string("null"))
                  << ",\"main_pid\":" << (service_info.main_pid.has_value() ? std::to_string(service_info.main_pid.value()) : std::string("null"))
                  << "}\n";
    } else {
        print_row("Name", service_info.name);
        print_row("Description", service_info.description);
        print_row("Load state", service_info.load_state);
        print_row("Active state", service_info.active_state);
        print_row("Sub state", service_info.sub_state);
        print_row("Loaded", service_info.loaded);
        print_row("Active", service_info.active);
        print_row("Enabled", service_info.enabled.has_value() ? (service_info.enabled.value() ? "true" : "false") : "unknown");
        print_row("Main PID", service_info.main_pid.has_value() ? std::to_string(service_info.main_pid.value()) : "none");
    }
    return 0;
}

int run_security_identity(const Options& options) {
    const auto result = security::identity();
    if (!result) {
        print_error(result.error(), options.json);
        return 1;
    }
    const auto& identity = result.value();
    if (options.json) {
        std::vector<std::string> group_strings;
        for (gid_t group : identity.groups) {
            group_strings.push_back(std::to_string(group));
        }
        std::cout << "{"
                  << "\"real_uid\":" << identity.real_uid
                  << ",\"effective_uid\":" << identity.effective_uid
                  << ",\"real_gid\":" << identity.real_gid
                  << ",\"effective_gid\":" << identity.effective_gid
                  << ",\"groups\":" << join_json_array(group_strings)
                  << ",\"is_root\":" << (identity.is_root ? "true" : "false")
                  << "}\n";
    } else {
        print_row("Real UID", std::to_string(identity.real_uid));
        print_row("Effective UID", std::to_string(identity.effective_uid));
        print_row("Real GID", std::to_string(identity.real_gid));
        print_row("Effective GID", std::to_string(identity.effective_gid));
        print_row("Groups", std::accumulate(identity.groups.begin(), identity.groups.end(), std::string(), [](std::string a, gid_t b) {
            return a.empty() ? std::to_string(b) : a + ", " + std::to_string(b);
        }));
        print_row("Is root", identity.is_root);
    }
    return 0;
}

int run_security_capabilities(const Options& options) {
    const auto result = security::capabilities();
    if (!result) {
        print_error(result.error(), options.json);
        return 1;
    }
    const auto& capabilities = result.value();
    if (options.json) {
        std::vector<std::string> items;
        for (const auto& capability : capabilities) {
            items.push_back(json_string(capability));
        }
        std::cout << join_json_array(items) << "\n";
    } else {
        for (const auto& capability : capabilities) {
            std::cout << capability << '\n';
        }
    }
    return 0;
}

int run_plugins_list(const Options& options, std::string_view directory) {
    const auto result = plugin::discover(directory);
    if (!result) {
        print_error(result.error(), options.json);
        return 1;
    }
    const auto& plugins = result.value();
    if (options.json) {
        std::vector<std::string> items;
        for (const auto& plugin_info : plugins) {
            items.push_back("{"
                "\"name\":" + json_string(plugin_info.name) +
                ",\"version\":" + json_string(plugin_info.version) +
                ",\"description\":" + json_string(plugin_info.description) +
                ",\"api_version\":" + json_string(plugin_info.api_version) +
                ",\"commands\":" + join_json_array(plugin_info.commands) +
                ",\"path\":" + json_string(plugin_info.path) +
                "}");
        }
        std::cout << join_json_array(items) << "\n";
    } else {
        for (const auto& plugin_info : plugins) {
            std::cout << plugin_info.name << "@" << plugin_info.version << " - " << plugin_info.description << '\n';
            if (!plugin_info.commands.empty()) {
                std::cout << "  commands: ";
                for (std::size_t i = 0; i < plugin_info.commands.size(); ++i) {
                    if (i > 0) {
                        std::cout << ", ";
                    }
                    std::cout << plugin_info.commands[i];
                }
                std::cout << '\n';
            }
        }
    }
    return 0;
}

int run_command(const Options& options) {
    if (options.args.empty()) {
        print_help();
        return 0;
    }

    const auto command = options.args[0];
    if (command == "system") {
        if (options.args.size() == 2 && options.args[1] == "info") {
            return run_system_info(options);
        }
        if (options.args.size() == 2 && options.args[1] == "cpu") {
            return run_system_cpu(options);
        }
        if (options.args.size() == 2 && options.args[1] == "memory") {
            return run_system_memory(options);
        }
        if (options.args.size() == 2 && options.args[1] == "load") {
            return run_system_load(options);
        }
        if (options.args.size() == 2 && options.args[1] == "modules") {
            return run_system_modules(options);
        }
        if (options.args.size() == 2 && options.args[1] == "hwmon") {
            return run_system_hwmon(options);
        }
    } else if (command == "process") {
        if (options.args.size() == 2 && options.args[1] == "list") {
            return run_process_list(options);
        }
        if (options.args.size() == 3 && options.args[1] == "inspect") {
            return run_process_inspect(options, options.args[2]);
        }
        if (options.args.size() == 3 && options.args[1] == "environment") {
            try {
                const auto pid = static_cast<pid_t>(std::stoll(std::string(options.args[2])));
                const auto result = process::environment(pid);
                if (!result) {
                    print_error(result.error(), options.json);
                    return 1;
                }
                const auto& env = result.value();
                if (options.json) {
                    std::vector<std::string> items;
                    for (const auto& [key, value] : env) {
                        items.push_back("{"
                            "\"key\":" + json_string(key) +
                            ",\"value\":" + json_string(value) +
                            "}");
                    }
                    std::cout << join_json_array(items) << "\n";
                } else {
                    for (const auto& [key, value] : env) {
                        std::cout << key << "=" << value << "\n";
                    }
                }
                return 0;
            } catch (const std::exception&) {
                std::cerr << "Invalid PID: " << options.args[2] << '\n';
                return 2;
            }
        }
        if (options.args.size() >= 3 && options.args[1] == "signal") {
            try {
                const auto pid = static_cast<pid_t>(std::stoll(std::string(options.args[2])));
                const int signal = std::stoi(options.args[3]);
                core::WriteOptions write_opts;
                write_opts.confirm_prompt = "Send signal " + std::to_string(signal) + " to PID " + std::to_string(pid) + "?";
                const auto result = process::send_signal(pid, signal, write_opts);
                if (!result) {
                    print_error(result.error(), options.json);
                    return 1;
                }
                if (!options.json) {
                    std::cout << "Sent signal " << signal << " to PID " << pid << "\n";
                }
                return 0;
            } catch (const std::exception&) {
                std::cerr << "Invalid arguments for signal command\n";
                return 2;
            }
        }
        if (options.args.size() >= 2 && options.args[1] == "terminate") {
            try {
                const auto pid = static_cast<pid_t>(std::stoll(std::string(options.args[2])));
                core::WriteOptions write_opts;
                write_opts.force = false;
                if (options.args.size() > 3 && options.args[3] == "--force") {
                    write_opts.force = true;
                }
                write_opts.confirm_prompt = "Terminate PID " + std::to_string(pid) + "?";
                const auto result = process::terminate(pid, write_opts);
                if (!result) {
                    print_error(result.error(), options.json);
                    return 1;
                }
                if (!options.json) {
                    std::cout << "Terminated PID " << pid << "\n";
                }
                return 0;
            } catch (const std::exception&) {
                std::cerr << "Invalid arguments for terminate command\n";
                return 2;
            }
        }
        if (options.args.size() == 3 && options.args[1] == "suspend") {
            try {
                const auto pid = static_cast<pid_t>(std::stoll(std::string(options.args[2])));
                core::WriteOptions write_opts;
                write_opts.confirm_prompt = "Suspend PID " + std::to_string(pid) + "?";
                const auto result = process::suspend(pid, write_opts);
                if (!result) {
                    print_error(result.error(), options.json);
                    return 1;
                }
                if (!options.json) {
                    std::cout << "Suspended PID " << pid << "\n";
                }
                return 0;
            } catch (const std::exception&) {
                std::cerr << "Invalid PID: " << options.args[2] << '\n';
                return 2;
            }
        }
        if (options.args.size() == 3 && options.args[1] == "resume") {
            try {
                const auto pid = static_cast<pid_t>(std::stoll(std::string(options.args[2])));
                core::WriteOptions write_opts;
                write_opts.confirm_prompt = "Resume PID " + std::to_string(pid) + "?";
                const auto result = process::resume(pid, write_opts);
                if (!result) {
                    print_error(result.error(), options.json);
                    return 1;
                }
                if (!options.json) {
                    std::cout << "Resumed PID " << pid << "\n";
                }
                return 0;
            } catch (const std::exception&) {
                std::cerr << "Invalid PID: " << options.args[2] << '\n';
                return 2;
            }
        }
        if (options.args.size() == 3 && options.args[1] == "nice") {
            try {
                const auto pid = static_cast<pid_t>(std::stoll(std::string(options.args[2])));
                const int nice_value = std::stoi(options.args[3]);
                core::WriteOptions write_opts;
                write_opts.confirm_prompt = "Set nice value of PID " + std::to_string(pid) + " to " + std::to_string(nice_value) + "?";
                const auto result = process::set_nice(pid, nice_value, write_opts);
                if (!result) {
                    print_error(result.error(), options.json);
                    return 1;
                }
                if (!options.json) {
                    std::cout << "Set nice value of PID " << pid << " to " << nice_value << "\n";
                }
                return 0;
            } catch (const std::exception&) {
                std::cerr << "Invalid arguments for nice command\n";
                return 2;
            }
        }
    } else if (command == "fs") {
        if (options.args.size() == 3 && options.args[1] == "usage") {
            return run_filesystem_usage(options, options.args[2]);
        }
        if (options.args.size() == 3 && options.args[1] == "info") {
            return run_filesystem_info(options, options.args[2]);
        }
        if (options.args.size() == 2 && options.args[1] == "mounts") {
            const auto result = filesystem::mounts();
            if (!result) {
                print_error(result.error(), options.json);
                return 1;
            }
            const auto& mounts = result.value();
            if (options.json) {
                std::vector<std::string> items;
                for (const auto& mount : mounts) {
                    items.push_back("{"
                        "\"device\":" + json_string(mount.device) +
                        ",\"mount_point\":" + json_string(mount.mount_point) +
                        ",\"filesystem_type\":" + json_string(mount.filesystem_type) +
                        ",\"options\":" + json_string(mount.options) +
                        ",\"dump\":" + std::to_string(mount.dump) +
                        ",\"pass\":" + std::to_string(mount.pass) +
                        "}");
                }
                std::cout << join_json_array(items) << "\n";
            } else {
                for (const auto& mount : mounts) {
                    std::cout << mount.device << " on " << mount.mount_point
                              << " type " << mount.filesystem_type
                              << " (" << mount.options << ")\n";
                }
            }
            return 0;
        }
        if (options.args.size() >= 2 && options.args[1] == "mkdir") {
            try {
                const auto path = options.args[2];
                core::WriteOptions write_opts;
                write_opts.recursive = false;
                mode_t permissions = 0755;
                
                // Parse optional flags
                for (std::size_t i = 3; i < options.args.size(); ++i) {
                    if (options.args[i] == "--recursive") {
                        write_opts.recursive = true;
                    } else if (options.args[i] == "--mode" && i + 1 < options.args.size()) {
                        permissions = static_cast<mode_t>(std::stoi(options.args[i + 1], nullptr, 8));
                        ++i;
                    }
                }
                
                const auto result = filesystem::create_directory(path, write_opts, permissions);
                if (!result) {
                    print_error(result.error(), options.json);
                    return 1;
                }
                if (!options.json) {
                    std::cout << "Created directory: " << path << "\n";
                }
                return 0;
            } catch (const std::exception&) {
                std::cerr << "Invalid arguments for mkdir command\n";
                return 2;
            }
        }
        if (options.args.size() >= 2 && options.args[1] == "rm") {
            try {
                const auto path = options.args[2];
                core::WriteOptions write_opts;
                write_opts.recursive = false;
                write_opts.confirm_prompt = "Remove '" + std::string(path) + "'?";
                
                for (std::size_t i = 3; i < options.args.size(); ++i) {
                    if (options.args[i] == "--recursive") {
                        write_opts.recursive = true;
                    }
                }
                
                const auto result = filesystem::remove(path, write_opts);
                if (!result) {
                    print_error(result.error(), options.json);
                    return 1;
                }
                if (!options.json) {
                    std::cout << "Removed: " << path << "\n";
                }
                return 0;
            } catch (const std::exception&) {
                std::cerr << "Invalid arguments for rm command\n";
                return 2;
            }
        }
        if (options.args.size() >= 3 && options.args[1] == "cp") {
            try {
                const auto from = options.args[2];
                const auto to = options.args[3];
                core::WriteOptions write_opts;
                write_opts.recursive = false;
                
                for (std::size_t i = 4; i < options.args.size(); ++i) {
                    if (options.args[i] == "--recursive") {
                        write_opts.recursive = true;
                    }
                }
                
                const auto result = filesystem::copy(from, to, write_opts);
                if (!result) {
                    print_error(result.error(), options.json);
                    return 1;
                }
                if (!options.json) {
                    std::cout << "Copied '" << from << "' to '" << to << "'\n";
                }
                return 0;
            } catch (const std::exception&) {
                std::cerr << "Invalid arguments for cp command\n";
                return 2;
            }
        }
        if (options.args.size() == 4 && options.args[1] == "mv") {
            try {
                const auto from = options.args[2];
                const auto to = options.args[3];
                core::WriteOptions write_opts;
                write_opts.confirm_prompt = "Move '" + std::string(from) + "' to '" + std::string(to) + "'?";
                
                const auto result = filesystem::rename(from, to, write_opts);
                if (!result) {
                    print_error(result.error(), options.json);
                    return 1;
                }
                if (!options.json) {
                    std::cout << "Moved '" << from << "' to '" << to << "'\n";
                }
                return 0;
            } catch (const std::exception&) {
                std::cerr << "Invalid arguments for mv command\n";
                return 2;
            }
        }
        if (options.args.size() >= 3 && options.args[1] == "write") {
            try {
                const auto path = options.args[2];
                std::string content;
                if (options.args.size() > 3) {
                    content = options.args[3];
                } else {
                    // Read from stdin if no content provided
                    std::string line;
                    while (std::getline(std::cin, line)) {
                        content += line + "\n";
                    }
                }
                
                core::WriteOptions write_opts;
                const auto result = filesystem::write_file(path, content, write_opts);
                if (!result) {
                    print_error(result.error(), options.json);
                    return 1;
                }
                if (!options.json) {
                    std::cout << "Wrote " << content.size() << " bytes to '" << path << "'\n";
                }
                return 0;
            } catch (const std::exception&) {
                std::cerr << "Invalid arguments for write command\n";
                return 2;
            }
        }
        if (options.args.size() == 3 && options.args[1] == "chmod") {
            try {
                const auto path = options.args[2];
                const auto mode = static_cast<mode_t>(std::stoi(options.args[3], nullptr, 8));
                core::WriteOptions write_opts;
                write_opts.confirm_prompt = "Change permissions of '" + std::string(path) + "' to " + options.args[3] + "?";
                
                const auto result = filesystem::set_permissions(path, mode, write_opts);
                if (!result) {
                    print_error(result.error(), options.json);
                    return 1;
                }
                if (!options.json) {
                    std::cout << "Changed permissions of '" << path << "' to " << options.args[3] << "\n";
                }
                return 0;
            } catch (const std::exception&) {
                std::cerr << "Invalid arguments for chmod command\n";
                return 2;
            }
        }
        if (options.args.size() == 3 && options.args[1] == "chown") {
            try {
                const auto path = options.args[2];
                const auto colon_pos = options.args[3].find(':');
                if (colon_pos == std::string::npos) {
                    std::cerr << "Invalid UID:GID format, expected UID:GID\n";
                    return 2;
                }
                
                const auto uid = static_cast<uid_t>(std::stoi(options.args[3].substr(0, colon_pos)));
                const auto gid = static_cast<gid_t>(std::stoi(options.args[3].substr(colon_pos + 1)));
                
                core::WriteOptions write_opts;
                write_opts.confirm_prompt = "Change owner of '" + std::string(path) + "' to " + options.args[3] + "?";
                
                const auto result = filesystem::set_owner(path, uid, gid, write_opts);
                if (!result) {
                    print_error(result.error(), options.json);
                    return 1;
                }
                if (!options.json) {
                    std::cout << "Changed owner of '" << path << "' to " << options.args[3] << "\n";
                }
                return 0;
            } catch (const std::exception&) {
                std::cerr << "Invalid arguments for chown command\n";
                return 2;
            }
        }
    } else if (command == "storage") {
        if (options.args.size() == 2 && options.args[1] == "list") {
            return run_storage_list(options);
        }
        if (options.args.size() == 3 && options.args[1] == "info") {
            return run_storage_info(options, options.args[2]);
        }
        if (options.args.size() == 3 && options.args[1] == "iostat") {
            const auto result = storage::iostat(options.args[2]);
            if (!result) {
                print_error(result.error(), options.json);
                return 1;
            }
            const auto& stats = result.value();
            if (options.json) {
                std::cout << "{"
                          << "\"device\":" << json_string(options.args[2])
                          << ",\"read_ops\":" << stats.read_ops
                          << ",\"write_ops\":" << stats.write_ops
                          << ",\"read_sectors\":" << stats.read_sectors
                          << ",\"write_sectors\":" << stats.write_sectors
                          << ",\"read_bytes\":" << stats.read_bytes
                          << ",\"write_bytes\":" << stats.write_bytes
                          << ",\"io_time_ms\":" << stats.io_time_ms
                          << "}\n";
            } else {
                print_row("Device", options.args[2]);
                print_row("Read operations", std::to_string(stats.read_ops));
                print_row("Write operations", std::to_string(stats.write_ops));
                print_row("Read sectors", std::to_string(stats.read_sectors));
                print_row("Write sectors", std::to_string(stats.write_sectors));
                print_row("Read bytes", std::to_string(stats.read_bytes));
                print_row("Write bytes", std::to_string(stats.write_bytes));
                print_row("IO time", std::to_string(stats.io_time_ms) + " ms");
            }
            return 0;
        }
    } else if (command == "network") {
        if (options.args.size() == 2 && options.args[1] == "interfaces") {
            return run_network_interfaces(options);
        }
        if (options.args.size() == 3 && options.args[1] == "info") {
            return run_network_info(options, options.args[2]);
        }
        if (options.args.size() == 2 && options.args[1] == "connections") {
            const auto result = network::connections();
            if (!result) {
                print_error(result.error(), options.json);
                return 1;
            }
            const auto& connections = result.value();
            if (options.json) {
                std::vector<std::string> items;
                for (const auto& conn : connections) {
                    items.push_back("{"
                        "\"protocol\":" + json_string(conn.protocol) +
                        ",\"local_address\":" + json_string(conn.local_address) +
                        ",\"local_port\":" + std::to_string(conn.local_port) +
                        ",\"remote_address\":" + json_string(conn.remote_address) +
                        ",\"remote_port\":" + std::to_string(conn.remote_port) +
                        ",\"state\":" + json_string(conn.state) +
                        ",\"uid\":" + std::to_string(conn.uid) +
                        ",\"inode\":" + std::to_string(conn.inode) +
                        "}");
                }
                std::cout << join_json_array(items) << "\n";
            } else {
                for (const auto& conn : connections) {
                    std::cout << conn.protocol << " "
                              << conn.local_address << ":" << conn.local_port
                              << " -> " << conn.remote_address << ":" << conn.remote_port
                              << " " << conn.state << "\n";
                }
            }
            return 0;
        }
    } else if (command == "service") {
        if (options.args.size() == 2 && options.args[1] == "list") {
            return run_service_list(options);
        }
        if (options.args.size() == 3 && (options.args[1] == "status" || options.args[1] == "inspect")) {
            return run_service_inspect(options, options.args[2]);
        }
        if (options.args.size() >= 3 && (options.args[1] == "start" || options.args[1] == "stop" || options.args[1] == "restart" || options.args[1] == "reload")) {
            try {
                const auto unit_name = options.args[2];
                service::ServiceAction action;
                if (options.args[1] == "start") {
                    action = service::ServiceAction::Start;
                } else if (options.args[1] == "stop") {
                    action = service::ServiceAction::Stop;
                } else if (options.args[1] == "restart") {
                    action = service::ServiceAction::Restart;
                } else {
                    action = service::ServiceAction::Reload;
                }
                
                core::WriteOptions write_opts;
                write_opts.confirm_prompt = std::string(options.args[1]) + " service '" + unit_name + "'?";
                
                const auto result = service::control(unit_name, action, write_opts);
                if (!result) {
                    print_error(result.error(), options.json);
                    return 1;
                }
                if (!options.json) {
                    std::cout << std::string(options.args[1]) + "ed service '" + unit_name + "'\n";
                }
                return 0;
            } catch (const std::exception&) {
                std::cerr << "Invalid arguments for service command\n";
                return 2;
            }
        }
        if (options.args.size() >= 3 && options.args[1] == "enable") {
            try {
                const auto unit_name = options.args[2];
                core::WriteOptions write_opts;
                write_opts.confirm_prompt = "Enable service '" + std::string(unit_name) + "' at boot?";
                
                const auto result = service::enable(unit_name, write_opts);
                if (!result) {
                    print_error(result.error(), options.json);
                    return 1;
                }
                if (!options.json) {
                    std::cout << "Enabled service '" << unit_name << "'\n";
                }
                return 0;
            } catch (const std::exception&) {
                std::cerr << "Invalid arguments for enable command\n";
                return 2;
            }
        }
        if (options.args.size() >= 3 && options.args[1] == "disable") {
            try {
                const auto unit_name = options.args[2];
                core::WriteOptions write_opts;
                write_opts.confirm_prompt = "Disable service '" + std::string(unit_name) + "' at boot?";
                
                const auto result = service::disable(unit_name, write_opts);
                if (!result) {
                    print_error(result.error(), options.json);
                    return 1;
                }
                if (!options.json) {
                    std::cout << "Disabled service '" << unit_name << "'\n";
                }
                return 0;
            } catch (const std::exception&) {
                std::cerr << "Invalid arguments for disable command\n";
                return 2;
            }
        }
    } else if (command == "security") {
        if (options.args.size() == 2 && options.args[1] == "identity") {
            return run_security_identity(options);
        }
        if (options.args.size() == 2 && options.args[1] == "capabilities") {
            return run_security_capabilities(options);
        }
    } else if (command == "plugins") {
        if (options.args.size() == 2 && options.args[1] == "list") {
            return run_plugins_list(options, "./plugins");
        }
        if (options.args.size() == 3 && options.args[1] == "list") {
            return run_plugins_list(options, options.args[2]);
        }
    }

    std::cerr << "Unknown or incomplete command." << '\n';
    print_help();
    return 2;
}

}  // namespace

int run(int argc, char* argv[]) {
    const auto options = parse_options(argc, argv);
    if (options.help) {
        print_help();
        return 0;
    }
    if (options.version) {
        std::cout << "nizaw " << core::version_string() << '\n';
        return 0;
    }
    return run_command(options);
}

}  // namespace nizaw::cli
