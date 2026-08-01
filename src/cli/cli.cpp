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
#include <iostream>
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

std::string to_lower(std::string_view value) {
    std::string result(value);
    std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c) { return std::tolower(c); });
    return result;
}

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

std::vector<std::string> split_tokens(std::string_view path) {
    std::vector<std::string> result;
    std::string current;
    for (char ch : path) {
        if (ch == ',' || ch == ';' || ch == ' ') {
            if (!current.empty()) {
                result.emplace_back(current);
                current.clear();
            }
        } else {
            current.push_back(ch);
        }
    }
    if (!current.empty()) {
        result.emplace_back(current);
    }
    return result;
}

void print_row(std::string_view label, std::string_view value) {
    std::cout << label << ": " << value << '\n';
}

void print_row(std::string_view label, bool value) {
    print_row(label, value ? "true" : "false");
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
              << "  process list\n"
              << "  process inspect <PID>\n"
              << "  fs usage <PATH>\n"
              << "  fs info <PATH>\n"
              << "  storage list\n"
              << "  storage info <DEVICE>\n"
              << "  network interfaces\n"
              << "  network info <IFACE>\n"
              << "  service list\n"
              << "  service status <UNIT>\n"
              << "  service inspect <UNIT>\n"
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
            std::cerr << "Unknown option: " << arg << '\n';
            options.help = true;
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
    } else if (command == "process") {
        if (options.args.size() == 2 && options.args[1] == "list") {
            return run_process_list(options);
        }
        if (options.args.size() == 3 && options.args[1] == "inspect") {
            return run_process_inspect(options, options.args[2]);
        }
    } else if (command == "fs") {
        if (options.args.size() == 3 && options.args[1] == "usage") {
            return run_filesystem_usage(options, options.args[2]);
        }
        if (options.args.size() == 3 && options.args[1] == "info") {
            return run_filesystem_info(options, options.args[2]);
        }
    } else if (command == "storage") {
        if (options.args.size() == 2 && options.args[1] == "list") {
            return run_storage_list(options);
        }
        if (options.args.size() == 3 && options.args[1] == "info") {
            return run_storage_info(options, options.args[2]);
        }
    } else if (command == "network") {
        if (options.args.size() == 2 && options.args[1] == "interfaces") {
            return run_network_interfaces(options);
        }
        if (options.args.size() == 3 && options.args[1] == "info") {
            return run_network_info(options, options.args[2]);
        }
    } else if (command == "service") {
        if (options.args.size() == 2 && options.args[1] == "list") {
            return run_service_list(options);
        }
        if (options.args.size() == 3 && (options.args[1] == "status" || options.args[1] == "inspect")) {
            return run_service_inspect(options, options.args[2]);
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
