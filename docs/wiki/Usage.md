# Usage

This guide provides comprehensive examples for using Nizaw both from the command line and from C++ code. Each section includes practical, real-world examples demonstrating proper error handling and best practices.

## Table of Contents

- [CLI Usage](#cli-usage)
- [Library Usage](#library-usage)
  - [System Information](#system-information)
  - [Process Management](#process-management)
  - [Filesystem Operations](#filesystem-operations)
  - [Storage Information](#storage-information)
  - [Network Information](#network-information)
  - [Service Management](#service-management)
  - [Security Context](#security-context)
  - [Plugin Discovery](#plugin-discovery)
  - [Error Handling Patterns](#error-handling-patterns)
  - [Working with Multiple Modules](#working-with-multiple-modules)
  - [Best Practices](#best-practices)

## CLI Usage

The CLI entry point is `nizaw` and follows a simple command tree.

### Global Flags

- `--help` / `-h` - Show usage and exit
- `--version` - Show version info and exit
- `--json` / `-j` - Emit machine-readable JSON instead of human-readable text
- `--verbose` / `-v` - Increase log verbosity (DEBUG level)
- `--quiet` - Suppress non-essential output (WARN and above only)
- `--no-color` - Disable ANSI color in output

### Commands

#### System

```bash
# View system information
./build/nizaw system info

# Output as JSON
./build/nizaw system info --json
```

#### Process

```bash
# List all processes
./build/nizaw process list

# Inspect a specific process by PID
./build/nizaw process inspect 1234
./build/nizaw process inspect --json 1234
```

#### Filesystem

```bash
# Check filesystem usage for a path
./build/nizaw fs usage /home
./build/nizaw fs usage --json /

# Get detailed filesystem information
./build/nizaw fs info /home/user/document.txt
./build/nizaw fs info --json /tmp
```

#### Storage

```bash
# List all block devices
./build/nizaw storage list

# Get detailed information about a specific device
./build/nizaw storage info sda
./build/nizaw storage info --json nvme0n1
```

#### Network

```bash
# List all network interfaces
./build/nizaw network interfaces

# Get detailed information about an interface
./build/nizaw network info eth0
./build/nizaw network info --json wlan0
```

#### Service

```bash
# List all systemd services
./build/nizaw service list

# Check service status
./build/nizaw service status ssh
./build/nizaw service status --json nginx

# Inspect a service in detail
./build/nizaw service inspect ssh
```

#### Security

```bash
# Get current process identity (UID/GID)
./build/nizaw security identity
./build/nizaw security identity --json

# List effective capabilities
./build/nizaw security capabilities
```

#### Plugins

```bash
# List plugins in default directory
./build/nizaw plugins list

# List plugins in custom directory
./build/nizaw plugins list /path/to/plugins
./build/nizaw plugins list --json ./custom/plugins
```

### JSON Output

All commands support `--json` for machine-readable output:

```bash
./build/nizaw system info --json
./build/nizaw process list --json
./build/nizaw storage info sda --json
```

JSON output is suitable for piping to tools like `jq`:

```bash
./build/nizaw system info --json | jq '.hostname'
./build/nizaw process list --json | jq '.[] | select(.name == "nginx")'
```

## Library Usage

### System Information

The `nizaw::system` module provides host and kernel information.

#### Basic Usage

```cpp
#include <iostream>
#include <nizaw/system.hpp>

int main() {
    // Get system information
    auto result = nizaw::system::info();
    
    // Always check for errors
    if (!result) {
        std::cerr << "Failed to get system info: " 
                  << result.error().message() << '\n';
        return 1;
    }
    
    // Access the data
    const auto& info = result.value();
    std::cout << "Hostname: " << info.hostname << '\n';
    std::cout << "Kernel: " << info.kernel_name << '\n';
    std::cout << "Kernel release: " << info.kernel_release << '\n';
    std::cout << "Architecture: " << info.architecture << '\n';
    std::cout << "CPU count: " << info.cpu_count << '\n';
    
    return 0;
}
```

#### Advanced Usage with CMake

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.20)
project(my_system_tool LANGUAGES CXX)

# Add Nizaw as a subdirectory
add_subdirectory(/path/to/nizaw nizaw_build)

# Link against specific modules
add_executable(my_tool main.cpp)
target_link_libraries(my_tool PRIVATE 
    nizaw::system 
    nizaw::core
)
```

```cpp
// main.cpp
#include <iostream>
#include <format>
#include <nizaw/system.hpp>

int main() {
    auto result = nizaw::system::info();
    if (!result) {
        std::cerr << "Error: " << result.error().message() << '\n';
        return 1;
    }
    
    const auto& sys = result.value();
    
    // Use modern C++ formatting
    std::cout << std::format("System: {} ({})\n", 
                             sys.hostname, 
                             sys.architecture);
    std::cout << std::format("Kernel: {} {}\n", 
                             sys.kernel_name, 
                             sys.kernel_release);
    std::cout << std::format("Uptime: {}\n", sys.uptime);
    
    return 0;
}
```

### Process Management

The `nizaw::process` module provides process enumeration and inspection.

#### Listing Processes

```cpp
#include <iostream>
#include <nizaw/process.hpp>

void list_processes(bool json_output = false) {
    auto result = nizaw::process::list();
    
    if (!result) {
        std::cerr << "Failed to list processes: " 
                  << result.error().message() << '\n';
        return;
    }
    
    const auto& processes = result.value();
    
    if (json_output) {
        std::cout << "[";
        for (size_t i = 0; i < processes.size(); ++i) {
            if (i > 0) std::cout << ",";
            const auto& p = processes[i];
            std::cout << "{"
                      << "\"pid\":" << p.pid << ","
                      << "\"name\":" << "\"" << p.name << "\","
                      << "\"state\":" << "\"" << p.state << "\""
                      << "}";
        }
        std::cout << "]\n";
    } else {
        std::cout << "PID\tNAME\tSTATE\n";
        for (const auto& p : processes) {
            std::cout << p.pid << "\t" 
                      << p.name << "\t" 
                      << p.state << '\n';
        }
    }
}
```

#### Inspecting a Process

```cpp
#include <iostream>
#include <nizaw/process.hpp>

void inspect_process(pid_t pid) {
    // Validate PID
    if (pid <= 0) {
        std::cerr << "Invalid PID: " << pid << '\n';
        return;
    }
    
    auto result = nizaw::process::inspect(pid);
    
    if (!result) {
        // Handle specific error codes
        const auto& err = result.error();
        if (err.code() == nizaw::ErrorCode::NotFound) {
            std::cerr << "Process " << pid << " not found\n";
        } else {
            std::cerr << "Failed to inspect process " << pid 
                      << ": " << err.message() << '\n';
        }
        return;
    }
    
    const auto& proc = result.value();
    
    std::cout << "Process Information:\n";
    std::cout << "  PID: " << proc.pid << '\n';
    std::cout << "  PPID: " << proc.ppid << '\n';
    std::cout << "  Name: " << proc.name << '\n';
    std::cout << "  State: " << proc.state << '\n';
    std::cout << "  User: " << proc.uid << '\n';
    std::cout << "  Group: " << proc.gid << '\n';
    std::cout << "  Threads: " << proc.threads << '\n';
    std::cout << "  Memory: " << proc.memory_kb << " KB\n";
    std::cout << "  CPU time: " << proc.cpu_time_seconds << "s\n";
    std::cout << "  Command: " << proc.command << '\n';
    std::cout << "  Executable: " << proc.executable << '\n';
}
```

#### Real-World Example: Process Monitor

```cpp
#include <chrono>
#include <iostream>
#include <map>
#include <nizaw/process.hpp>

class ProcessMonitor {
private:
    std::map<pid_t, std::string> previous_processes_;
    
public:
    void snapshot() {
        auto result = nizaw::process::list();
        if (!result) {
            std::cerr << "Error: " << result.error().message() << '\n';
            return;
        }
        
        std::map<pid_t, std::string> current;
        for (const auto& p : result.value()) {
            current[p.pid] = p.name;
        }
        
        // Find new processes
        for (const auto& [pid, name] : current) {
            if (previous_processes_.find(pid) == previous_processes_.end()) {
                std::cout << "[NEW] PID " << pid << ": " << name << '\n';
            }
        }
        
        // Find terminated processes
        for (const auto& [pid, name] : previous_processes_) {
            if (current.find(pid) == current.end()) {
                std::cout << "[EXIT] PID " << pid << ": " << name << '\n';
            }
        }
        
        previous_processes_ = std::move(current);
    }
};

int main() {
    ProcessMonitor monitor;
    
    while (true) {
        monitor.snapshot();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    return 0;
}
```

### Filesystem Operations

The `nizaw::filesystem` module provides disk usage and file metadata.

#### Checking Disk Usage

```cpp
#include <iostream>
#include <filesystem>
#include <nizaw/filesystem.hpp>

void check_disk_usage(const std::string& path) {
    auto result = nizaw::filesystem::usage(path);
    
    if (!result) {
        std::cerr << "Failed to get disk usage for '" << path 
                  << "': " << result.error().message() << '\n';
        return;
    }
    
    const auto& usage = result.value();
    
    // Convert bytes to human-readable format
    auto to_human = [](uint64_t bytes) {
        const char* units[] = {"B", "KB", "MB", "GB", "TB"};
        int unit = 0;
        double size = static_cast<double>(bytes);
        
        while (size >= 1024 && unit < 4) {
            size /= 1024;
            unit++;
        }
        
        return std::format("{:.2f} {}", size, units[unit]);
    };
    
    std::cout << "Disk usage for: " << path << '\n';
    std::cout << "  Total: " << to_human(usage.total_bytes) << '\n';
    std::cout << "  Used: " << to_human(usage.used_bytes) << '\n';
    std::cout << "  Free: " << to_human(usage.free_bytes) << '\n';
    std::cout << "  Available: " << to_human(usage.available_bytes) << '\n';
    
    // Calculate percentage
    double usage_percent = (static_cast<double>(usage.used_bytes) / 
                           usage.total_bytes) * 100.0;
    std::cout << "  Usage: " << std::format("{:.1f}%", usage_percent) << '\n';
}

int main() {
    check_disk_usage("/home");
    check_disk_usage("/var");
    return 0;
}
```

#### Getting File Information

```cpp
#include <iostream>
#include <nizaw/filesystem.hpp>

void analyze_file(const std::string& filepath) {
    auto result = nizaw::filesystem::info(filepath);
    
    if (!result) {
        std::cerr << "Failed to get info for '" << filepath 
                  << "': " << result.error().message() << '\n';
        return;
    }
    
    const auto& info = result.value();
    
    std::cout << "File Information:\n";
    std::cout << "  Path: " << info.path << '\n';
    std::cout << "  Type: " << info.type << '\n';
    std::cout << "  Permissions: " << info.permissions << '\n';
    std::cout << "  Owner: " << info.owner << '\n';
    std::cout << "  Group: " << info.group << '\n';
    std::cout << "  Size: " << info.size_bytes << " bytes\n";
    std::cout << "  Inode: " << info.inode << '\n';
    std::cout << "  Exists: " << (info.exists ? "yes" : "no") << '\n';
    std::cout << "  Symlink: " << (info.is_symlink ? "yes" : "no") << '\n';
    
    if (info.is_symlink) {
        std::cout << "  Mount point: " << info.mount_point << '\n';
    }
}

int main() {
    analyze_file("/etc/passwd");
    analyze_file("/tmp/test.txt");
    return 0;
}
```

### Storage Information

The `nizaw::storage` module enumerates block devices and provides detailed metadata.

```cpp
#include <iostream>
#include <algorithm>
#include <nizaw/storage.hpp>

void list_storage_devices() {
    auto result = nizaw::storage::enumerate();
    
    if (!result) {
        std::cerr << "Failed to enumerate storage: " 
                  << result.error().message() << '\n';
        return;
    }
    
    const auto& devices = result.value();
    
    std::cout << std::left << std::setw(15) << "NAME" 
              << std::setw(20) << "DEVICE" 
              << std::setw(25) << "MODEL" 
              << "SIZE\n";
    std::cout << std::string(80, '-') << '\n';
    
    for (const auto& dev : devices) {
        std::cout << std::left << std::setw(15) << dev.name
                  << std::setw(20) << dev.dev_node
                  << std::setw(25) << dev.model
                  << (dev.size_bytes / (1024*1024*1024)) << " GB\n";
    }
}

void inspect_device(const std::string& device_name) {
    auto result = nizaw::storage::inspect(device_name);
    
    if (!result) {
        std::cerr << "Failed to inspect device '" << device_name 
                  << "': " << result.error().message() << '\n';
        return;
    }
    
    const auto& dev = result.value();
    
    std::cout << "Device: " << dev.name << '\n';
    std::cout << "  Model: " << dev.model << '\n';
    std::cout << "  Vendor: " << dev.vendor << '\n';
    std::cout << "  Size: " << (dev.size_bytes / (1024*1024*1024)) << " GB\n";
    std::cout << "  Logical block size: " << dev.logical_block_size << " bytes\n";
    std::cout << "  Physical block size: " << dev.physical_block_size << " bytes\n";
    std::cout << "  Removable: " << (dev.removable ? "yes" : "no") << '\n';
    std::cout << "  Read-only: " << (dev.read_only ? "yes" : "no") << '\n';
    std::cout << "  Rotational: " << (dev.rotational ? "yes" : "no") << '\n';
}
```

### Network Information

The `nizaw::network` module provides network interface information.

```cpp
#include <iostream>
#include <iomanip>
#include <nizaw/network.hpp>

void list_interfaces() {
    auto result = nizaw::network::list();
    
    if (!result) {
        std::cerr << "Failed to list interfaces: " 
                  << result.error().message() << '\n';
        return;
    }
    
    std::cout << std::left << std::setw(15) << "INTERFACE"
              << std::setw(12) << "STATE"
              << std::setw(20) << "MAC ADDRESS"
              << "MTU\n";
    std::cout << std::string(60, '-') << '\n';
    
    for (const auto& iface : result.value()) {
        std::cout << std::left << std::setw(15) << iface.name
                  << std::setw(12) << iface.state
                  << std::setw(20) << iface.mac_address
                  << iface.mtu << '\n';
    }
}

void inspect_interface(const std::string& iface_name) {
    auto result = nizaw::network::inspect(iface_name);
    
    if (!result) {
        std::cerr << "Failed to inspect interface '" << iface_name 
                  << "': " << result.error().message() << '\n';
        return;
    }
    
    const auto& iface = result.value();
    
    std::cout << "Interface: " << iface.name << '\n';
    std::cout << "  Index: " << iface.index << '\n';
    std::cout << "  State: " << iface.state << '\n';
    std::cout << "  MAC: " << iface.mac_address << '\n';
    std::cout << "  MTU: " << iface.mtu << '\n';
    std::cout << "  Flags: ";
    for (size_t i = 0; i < iface.flags.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << iface.flags[i];
    }
    std::cout << '\n';
    
    std::cout << "  Addresses:\n";
    for (const auto& addr : iface.addresses) {
        std::cout << "    " << addr.family << ": " 
                  << addr.address << '\n';
        std::cout << "      Netmask: " << addr.netmask << '\n';
        if (!addr.broadcast.empty()) {
            std::cout << "      Broadcast: " << addr.broadcast << '\n';
        }
    }
}
```

### Service Management

The `nizaw::service` module provides systemd service information.

```cpp
#include <iostream>
#include <nizaw/service.hpp>

void list_services() {
    auto result = nizaw::service::list();
    
    if (!result) {
        std::cerr << "Failed to list services: " 
                  << result.error().message() << '\n';
        return;
    }
    
    std::cout << std::left << std::setw(30) << "NAME"
              << std::setw(15) << "ACTIVE"
              << "LOADED\n";
    std::cout << std::string(60, '-') << '\n';
    
    for (const auto& svc : result.value()) {
        std::cout << std::left << std::setw(30) << svc.name
                  << std::setw(15) << svc.active_state
                  << (svc.loaded ? "yes" : "no") << '\n';
    }
}

void inspect_service(const std::string& unit_name) {
    auto result = nizaw::service::inspect(unit_name);
    
    if (!result) {
        std::cerr << "Failed to inspect service '" << unit_name 
                  << "': " << result.error().message() << '\n';
        return;
    }
    
    const auto& svc = result.value();
    
    std::cout << "Service: " << svc.name << '\n';
    std::cout << "  Description: " << svc.description << '\n';
    std::cout << "  Load state: " << svc.load_state << '\n';
    std::cout << "  Active state: " << svc.active_state << '\n';
    std::cout << "  Sub state: " << svc.sub_state << '\n';
    std::cout << "  Loaded: " << (svc.loaded ? "yes" : "no") << '\n';
    std::cout << "  Active: " << (svc.active ? "yes" : "no") << '\n';
    
    if (svc.enabled.has_value()) {
        std::cout << "  Enabled: " 
                  << (svc.enabled.value() ? "yes" : "no") << '\n';
    } else {
        std::cout << "  Enabled: unknown\n";
    }
    
    if (svc.main_pid.has_value()) {
        std::cout << "  Main PID: " << svc.main_pid.value() << '\n';
    } else {
        std::cout << "  Main PID: none\n";
    }
}
```

### Security Context

The `nizaw::security` module provides identity and capability information.

```cpp
#include <iostream>
#include <nizaw/security.hpp>

void print_identity() {
    auto result = nizaw::security::identity();
    
    if (!result) {
        std::cerr << "Failed to get identity: " 
                  << result.error().message() << '\n';
        return;
    }
    
    const auto& identity = result.value();
    
    std::cout << "Process Identity:\n";
    std::cout << "  Real UID: " << identity.real_uid << '\n';
    std::cout << "  Effective UID: " << identity.effective_uid << '\n';
    std::cout << "  Real GID: " << identity.real_gid << '\n';
    std::cout << "  Effective GID: " << identity.effective_gid << '\n';
    std::cout << "  Root: " << (identity.is_root ? "yes" : "no") << '\n';
    
    std::cout << "  Groups: ";
    for (size_t i = 0; i < identity.groups.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << identity.groups[i];
    }
    std::cout << '\n';
}

void print_capabilities() {
    auto result = nizaw::security::capabilities();
    
    if (!result) {
        std::cerr << "Failed to get capabilities: " 
                  << result.error().message() << '\n';
        return;
    }
    
    const auto& capabilities = result.value();
    
    std::cout << "Effective capabilities:\n";
    for (const auto& cap : capabilities) {
        std::cout << "  - " << cap << '\n';
    }
}
```

### Plugin Discovery

The `nizaw::plugin` module allows discovering and loading plugins.

```cpp
#include <iostream>
#include <nizaw/plugin.hpp>

void discover_plugins(const std::string& directory) {
    // One-shot discovery
    auto result = nizaw::plugin::discover(directory);
    
    if (!result) {
        std::cerr << "Failed to discover plugins: " 
                  << result.error().message() << '\n';
        return;
    }
    
    const auto& plugins = result.value();
    
    if (plugins.empty()) {
        std::cout << "No plugins found in " << directory << '\n';
        return;
    }
    
    std::cout << "Found " << plugins.size() << " plugin(s):\n\n";
    
    for (const auto& plugin : plugins) {
        std::cout << "Name: " << plugin.name << '\n';
        std::cout << "  Version: " << plugin.version << '\n';
        std::cout << "  Description: " << plugin.description << '\n';
        std::cout << "  API Version: " << plugin.api_version << '\n';
        std::cout << "  Path: " << plugin.path << '\n';
        std::cout << "  Commands: ";
        for (size_t i = 0; i < plugin.commands.size(); ++i) {
            if (i > 0) std::cout << ", ";
            std::cout << plugin.commands[i];
        }
        std::cout << "\n\n";
    }
}

void use_plugin_registry(const std::string& directory) {
    nizaw::plugin::Registry registry;
    
    // Load plugins from directory
    auto load_result = registry.load_directory(directory);
    if (!load_result) {
        std::cerr << "Failed to load plugins: " 
                  << load_result.error().message() << '\n';
        return;
    }
    
    // Check if any plugins were loaded
    if (registry.empty()) {
        std::cout << "No plugins loaded\n";
        return;
    }
    
    // Access loaded plugins
    for (const auto& plugin : registry.plugins()) {
        std::cout << plugin.name << " v" << plugin.version << '\n';
    }
    
    // Registry will automatically unload plugins when it goes out of scope
}
```

### Error Handling Patterns

Proper error handling is essential when using Nizaw. Here are common patterns:

#### Basic Error Handling

```cpp
#include <iostream>
#include <nizaw/system.hpp>

int main() {
    auto result = nizaw::system::info();
    
    if (!result) {
        // Always handle errors
        std::cerr << "Error: " << result.error().message() << '\n';
        return 1;
    }
    
    // Safe to use result.value()
    const auto& info = result.value();
    std::cout << info.hostname << '\n';
    
    return 0;
}
```

#### Checking Error Codes

```cpp
#include <iostream>
#include <nizaw/process.hpp>

void safe_process_inspect(pid_t pid) {
    auto result = nizaw::process::inspect(pid);
    
    if (!result) {
        const auto& error = result.error();
        
        // Check specific error codes
        switch (error.code()) {
            case nizaw::ErrorCode::NotFound:
                std::cerr << "Process not found (may have exited)\n";
                break;
            case nizaw::ErrorCode::PermissionDenied:
                std::cerr << "Permission denied - try running with sudo\n";
                break;
            case nizaw::ErrorCode::IoError:
                std::cerr << "I/O error: " << error.message() << '\n';
                break;
            default:
                std::cerr << "Unexpected error (" 
                          << static_cast<int>(error.code()) 
                          << "): " << error.message() << '\n';
        }
        
        return;
    }
    
    // Process successfully inspected
    const auto& proc = result.value();
    std::cout << "Process: " << proc.name << " (PID: " << proc.pid << ")\n";
}
```

#### Error Propagation

```cpp
#include <iostream>
#include <nizaw/system.hpp>
#include <nizaw/process.hpp>

// Function that propagates errors using Result
nizaw::Result<std::string> get_hostname_with_fallback() {
    auto result = nizaw::system::info();
    if (!result) {
        return result.error();  // Propagate error
    }
    
    if (result.value().hostname.empty()) {
        return nizaw::Error(
            nizaw::ErrorCode::NotFound,
            "Hostname is empty",
            "system"
        );
    }
    
    return result.value().hostname;  // Return success value
}

int main() {
    auto hostname_result = get_hostname_with_fallback();
    
    if (!hostname_result) {
        std::cerr << "Cannot get hostname: " 
                  << hostname_result.error().message() << '\n';
        return 1;
    }
    
    std::cout << "Hostname: " << hostname_result.value() << '\n';
    return 0;
}
```

### Working with Multiple Modules

Real-world applications often need to combine multiple modules:

```cpp
#include <iostream>
#include <vector>
#include <algorithm>
#include <nizaw/system.hpp>
#include <nizaw/process.hpp>
#include <nizaw/storage.hpp>

struct SystemSnapshot {
    nizaw::system::SystemInfo system;
    std::vector<nizaw::process::ProcessInfo> processes;
    std::vector<nizaw::storage::Device> devices;
};

// Collect information from multiple modules
nizaw::Result<SystemSnapshot> collect_system_snapshot() {
    SystemSnapshot snapshot;
    
    // Get system info
    auto sys_result = nizaw::system::info();
    if (!sys_result) {
        return sys_result.error();
    }
    snapshot.system = sys_result.value();
    
    // Get process list
    auto proc_result = nizaw::process::list();
    if (!proc_result) {
        return proc_result.error();
    }
    snapshot.processes = std::move(proc_result.value());
    
    // Get storage devices
    auto storage_result = nizaw::storage::enumerate();
    if (!storage_result) {
        return storage_result.error();
    }
    snapshot.devices = std::move(storage_result.value());
    
    return snapshot;
}

void print_dashboard(const SystemSnapshot& snapshot) {
    std::cout << "=== System Dashboard ===\n\n";
    
    std::cout << "Host: " << snapshot.system.hostname << '\n';
    std::cout << "Kernel: " << snapshot.system.kernel_name 
              << " " << snapshot.system.kernel_release << '\n';
    std::cout << "CPU cores: " << snapshot.system.cpu_count << '\n';
    std::cout << '\n';
    
    std::cout << "Running processes: " << snapshot.processes.size() << '\n';
    
    // Count processes by state
    std::map<std::string, int> state_counts;
    for (const auto& p : snapshot.processes) {
        state_counts[p.state]++;
    }
    std::cout << "Process states:\n";
    for (const auto& [state, count] : state_counts) {
        std::cout << "  " << state << ": " << count << '\n';
    }
    std::cout << '\n';
    
    std::cout << "Storage devices: " << snapshot.devices.size() << '\n';
    uint64_t total_storage = 0;
    for (const auto& dev : snapshot.devices) {
        total_storage += dev.size_bytes;
    }
    std::cout << "Total storage: " << (total_storage / (1024*1024*1024)) 
              << " GB\n";
}

int main() {
    auto snapshot_result = collect_system_snapshot();
    
    if (!snapshot_result) {
        std::cerr << "Failed to collect system snapshot: " 
                  << snapshot_result.error().message() << '\n';
        return 1;
    }
    
    print_dashboard(snapshot_result.value());
    
    return 0;
}
```

### Best Practices

#### 1. Always Check for Errors

```cpp
// BAD: Ignoring errors
auto result = nizaw::system::info();
std::cout << result.value().hostname << '\n';  // May crash!

// GOOD: Always check
auto result = nizaw::system::info();
if (!result) {
    std::cerr << "Error: " << result.error().message() << '\n';
    return 1;
}
std::cout << result.value().hostname << '\n';
```

#### 2. Use value() Carefully

```cpp
// value() throws std::logic_error if the Result contains an error
// Only call it after checking has_value() or using operator bool()

auto result = nizaw::system::info();
if (result) {  // Operator bool checks if value exists
    std::cout << result.value().hostname << '\n';
}

// Or use value_or() for default values
std::string hostname = result.value_or("unknown");
```

#### 3. Handle Specific Error Codes

```cpp
auto result = nizaw::process::inspect(pid);
if (!result) {
    const auto& error = result.error();
    
    // Check error code
    if (error.code() == nizaw::ErrorCode::PermissionDenied) {
        // Handle permission errors specifically
        std::cerr << "Permission denied\n";
    } else if (error.code() == nizaw::ErrorCode::NotFound) {
        // Handle not found
        std::cerr << "Process not found\n";
    } else {
        // Generic error handling
        std::cerr << "Error: " << error.message() << '\n';
    }
}
```

#### 4. Use const References for Performance

```cpp
// GOOD: Use const reference to avoid copying
if (result) {
    const auto& data = result.value();
    process_data(data);
}

// BAD: Unnecessary copy
if (result) {
    auto data = result.value();  // Copies the entire struct
    process_data(data);
}
```

#### 5. Combine Multiple Module Results

```cpp
nizaw::Result<SystemSnapshot> collect_all() {
    SystemSnapshot snapshot;
    
    auto sys = nizaw::system::info();
    if (!sys) return sys.error();
    snapshot.system = sys.value();
    
    auto procs = nizaw::process::list();
    if (!procs) return procs.error();
    snapshot.processes = procs.value();
    
    // Continue with other modules...
    
    return snapshot;
}
```

#### 6. Use RAII for Resource Management

```cpp
// The Registry class automatically unloads plugins
void load_plugins_safely() {
    nizaw::plugin::Registry registry;
    
    auto result = registry.load_directory("./plugins");
    if (!result) {
        std::cerr << "Error: " << result.error().message() << '\n';
        return;  // Registry destructor automatically unloads plugins
    }
    
    // Use plugins...
    
}  // Plugins automatically unloaded here
```

## Real-World Examples

### System Monitoring Tool

```cpp
#include <chrono>
#include <iostream>
#include <thread>
#include <nizaw/system.hpp>
#include <nizaw/process.hpp>

class SystemMonitor {
private:
    std::chrono::steady_clock::time_point start_;
    
public:
    SystemMonitor() : start_(std::chrono::steady_clock::now()) {}
    
    void monitor() {
        while (true) {
            auto sys_result = nizaw::system::info();
            auto proc_result = nizaw::process::list();
            
            if (sys_result && proc_result) {
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                    std::chrono::steady_clock::now() - start_
                );
                
                std::cout << "\033[2J\033[H";  // Clear screen
                std::cout << "=== System Monitor (Uptime: " 
                          << elapsed.count() << "s) ===\n\n";
                
                std::cout << "Host: " << sys_result.value().hostname << '\n';
                std::cout << "CPU cores: " << sys_result.value().cpu_count << '\n';
                std::cout << "Running processes: " 
                          << proc_result.value().size() << '\n';
            }
            
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
};

int main() {
    SystemMonitor monitor;
    monitor.monitor();
    return 0;
}
```

### Disk Space Alert System

```cpp
#include <iostream>
#include <map>
#include <nizaw/filesystem.hpp>

class DiskSpaceMonitor {
private:
    std::map<std::string, double> thresholds_;
    
public:
    void set_threshold(const std::string& path, double percent) {
        thresholds_[path] = percent;
    }
    
    void check() {
        for (const auto& [path, threshold] : thresholds_) {
            auto result = nizaw::filesystem::usage(path);
            
            if (!result) {
                std::cerr << "Error checking " << path << ": "
                          << result.error().message() << '\n';
                continue;
            }
            
            const auto& usage = result.value();
            double used_percent = (static_cast<double>(usage.used_bytes) / 
                                  usage.total_bytes) * 100.0;
            
            std::cout << std::format("{}: {:.1f}% used", path, used_percent);
            
            if (used_percent > threshold) {
                std::cout << " - WARNING: Above threshold (" 
                          << threshold << "%)\n";
            } else {
                std::cout << " - OK\n";
            }
        }
    }
};

int main() {
    DiskSpaceMonitor monitor;
    monitor.set_threshold("/", 90.0);
    monitor.set_threshold("/home", 80.0);
    monitor.set_threshold("/var", 85.0);
    
    monitor.check();
    
    return 0;
}
```

### Service Health Checker

```cpp
#include <iostream>
#include <set>
#include <nizaw/service.hpp>

class ServiceHealthChecker {
private:
    std::set<std::string> critical_services_;
    
public:
    void add_critical(const std::string& service) {
        critical_services_.insert(service);
    }
    
    void check() {
        auto result = nizaw::service::list();
        
        if (!result) {
            std::cerr << "Failed to list services: "
                      << result.error().message() << '\n';
            return;
        }
        
        bool all_ok = true;
        
        for (const auto& svc : result.value()) {
            if (critical_services_.count(svc.name)) {
                bool is_healthy = svc.active && svc.loaded;
                
                std::cout << std::format("{}: {}", 
                    svc.name, 
                    is_healthy ? "OK" : "FAILED");
                
                if (!is_healthy) {
                    std::cout << std::format(" (state: {}, loaded: {})",
                        svc.active_state,
                        svc.loaded ? "yes" : "no");
                    all_ok = false;
                }
                
                std::cout << '\n';
            }
        }
        
        if (all_ok) {
            std::cout << "All critical services are healthy\n";
        } else {
            std::cout << "WARNING: Some services are not healthy\n";
        }
    }
};

int main() {
    ServiceHealthChecker checker;
    checker.add_critical("sshd");
    checker.add_critical("nginx");
    checker.add_critical("postgresql");
    
    checker.check();
    
    return 0;
}
```

## See Also

- [CLI Design](../cli-design.md) - Complete CLI command reference
- [API Design](../api-design.md) - Detailed API specifications
- [Architecture](../architecture.md) - System architecture and design
- [Plugin Development](../wiki/PluginDevelopment.md) - Creating custom plugins
- [Integration](../wiki/Integration.md) - Integrating Nizaw into your project