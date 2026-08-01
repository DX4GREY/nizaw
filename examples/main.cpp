#include <algorithm>
#include <iomanip>
#include <iostream>
#include <string>
#include <vector>

#include <nizaw/core/env.hpp>
#include <nizaw/core/platform.hpp>
#include <nizaw/core/version.hpp>
#include <nizaw/filesystem.hpp>
#include <nizaw/network.hpp>
#include <nizaw/process.hpp>
#include <nizaw/security.hpp>
#include <nizaw/storage.hpp>
#include <nizaw/system.hpp>

static void print_core(bool json) {
    const auto version = nizaw::core::version();
    const auto plat = nizaw::core::detect();
    const auto home = nizaw::core::env::get_or("HOME", "(unset)");

    if (json) {
        std::cout << "\"core\": {"
                  << "\"version\": \"" << version.major << "." << version.minor << "." << version.patch << "\","
                  << "\"distro_id\": \"" << plat.distro_id << "\","
                  << "\"distro_name\": \"" << plat.distro_name << "\","
                  << "\"has_systemd\": " << (plat.has_systemd ? "true" : "false") << ","
                  << "\"home\": \"" << home << "\"}\n";
    } else {
        std::cout << "Core\n";
        std::cout << "────────────────────────────\n";
        std::cout << "Version:       " << version.major << "." << version.minor << "." << version.patch << '\n';
        std::cout << "Distro:        " << plat.distro_name << " (" << plat.distro_id << ")\n";
        std::cout << "Systemd:       " << (plat.has_systemd ? "yes" : "no") << '\n';
        std::cout << "HOME:          " << home << '\n';
    }
}

static void print_system(bool json) {
    auto res = nizaw::system::info();
    if (!res) {
        std::cerr << "system: " << res.error().message() << '\n';
        return;
    }
    const auto &s = res.value();
    if (json) {
        std::cout << "\"system\": {\n";
        std::cout << "  \"hostname\": \"" << s.hostname << "\",\n";
        std::cout << "  \"kernel_name\": \"" << s.kernel_name << "\",\n";
        std::cout << "  \"kernel_release\": \"" << s.kernel_release << "\",\n";
        std::cout << "  \"kernel_version\": \"" << s.kernel_version << "\",\n";
        std::cout << "  \"architecture\": \"" << s.architecture << "\",\n";
        std::cout << "  \"uptime\": \"" << s.uptime << "\",\n";
        std::cout << "  \"boot_time\": \"" << s.boot_time << "\",\n";
        std::cout << "  \"page_size\": " << s.page_size << ",\n";
        std::cout << "  \"cpu_count\": " << s.cpu_count << "\n";
        std::cout << "}\n";
    } else {
        std::cout << "\nSystem\n";
        std::cout << "────────────────────────────\n";
        std::cout << "Hostname:       " << s.hostname << '\n';
        std::cout << "Kernel name:    " << s.kernel_name << '\n';
        std::cout << "Kernel release: " << s.kernel_release << '\n';
        std::cout << "Kernel version: " << s.kernel_version << '\n';
        std::cout << "Architecture:   " << s.architecture << '\n';
        std::cout << "Uptime:         " << s.uptime << '\n';
        std::cout << "Boot time:      " << s.boot_time << '\n';
        std::cout << "Page size:      " << s.page_size << '\n';
        std::cout << "CPU count:      " << s.cpu_count << '\n';
    }
}

static void print_processes(bool json, size_t limit = 5) {
    auto res = nizaw::process::list();
    if (!res) {
        std::cerr << "process.list: " << res.error().message() << '\n';
        return;
    }
    const auto &list = res.value();
    if (json) {
        std::cout << "\"processes\": [\n";
        for (size_t i = 0; i < list.size() && i < limit; ++i) {
            const auto &p = list[i];
            std::cout << "  { \"pid\": " << p.pid
                      << ", \"name\": \"" << p.name
                      << "\", \"state\": \"" << p.state
                      << "\", \"command\": \"" << p.command << "\" }";
            if (i + 1 < list.size() && i + 1 < limit) std::cout << ",";
            std::cout << "\n";
        }
        std::cout << "]\n";
    } else {
        std::cout << "\nTop " << std::min(limit, list.size()) << " processes\n";
        std::cout << "PID\tNAME\tSTATE\tCOMMAND\n";
        for (size_t i = 0; i < list.size() && i < limit; ++i) {
            const auto &p = list[i];
            std::cout << p.pid << '\t' << p.name << '\t' << p.state << '\t' << p.command << '\n';
        }
    }
}

static void print_disk_usage(bool json, const std::string &path = "/") {
    auto res = nizaw::filesystem::usage(path);
    if (!res) {
        std::cerr << "fs.usage(" << path << "): " << res.error().message() << '\n';
        return;
    }
    const auto &d = res.value();
    if (json) {
        std::cout << "\"disk_usage\": {"
                  << "\"total\":" << d.total_bytes << ","
                  << "\"free\":" << d.free_bytes << ","
                  << "\"available\":" << d.available_bytes << ","
                  << "\"used\":" << d.used_bytes << "}\n";
    } else {
        std::cout << "\nDisk usage for " << path << '\n';
        std::cout << "Total:     " << d.total_bytes << " bytes\n";
        std::cout << "Free:      " << d.free_bytes << " bytes\n";
        std::cout << "Available: " << d.available_bytes << " bytes\n";
        std::cout << "Used:      " << d.used_bytes << " bytes\n";
    }
}

static void print_network(bool json) {
    auto res = nizaw::network::list();
    if (!res) {
        std::cerr << "network.list: " << res.error().message() << '\n';
        return;
    }
    const auto &ifs = res.value();
    if (json) {
        std::cout << "\"interfaces\": [\n";
        for (size_t i = 0; i < ifs.size(); ++i) {
            const auto &it = ifs[i];
            std::cout << "  { \"name\": \"" << it.name
                      << "\", \"state\": \"" << it.state
                      << "\", \"mac_address\": \"" << it.mac_address
                      << "\", \"mtu\": " << it.mtu
                      << ", \"addresses\": [";
            for (size_t j = 0; j < it.addresses.size(); ++j) {
                const auto &addr = it.addresses[j];
                std::cout << "{ \"family\": \"" << addr.family
                          << "\", \"address\": \"" << addr.address
                          << "\", \"netmask\": \"" << addr.netmask
                          << "\", \"broadcast\": \"" << addr.broadcast << "\" }";
                if (j + 1 < it.addresses.size()) std::cout << ", ";
            }
            std::cout << "] }";
            if (i + 1 < ifs.size()) std::cout << ",";
            std::cout << "\n";
        }
        std::cout << "]\n";
    } else {
        std::cout << "\nNetwork interfaces\n";
        for (const auto &it : ifs) {
            std::cout << it.name << " (" << it.state << ")\n";
            std::cout << "  MAC: " << it.mac_address << "  MTU: " << it.mtu << '\n';
            for (const auto &addr : it.addresses) {
                std::cout << "  " << addr.family << ": " << addr.address;
                if (!addr.netmask.empty()) std::cout << " (netmask " << addr.netmask << ")";
                if (!addr.broadcast.empty()) std::cout << " (broadcast " << addr.broadcast << ")";
                std::cout << '\n';
            }
        }
    }
}

static void print_identity(bool json) {
    auto res = nizaw::security::identity();
    if (!res) {
        std::cerr << "security.identity: " << res.error().message() << '\n';
        return;
    }
    const auto &id = res.value();
    if (json) {
        std::cout << "\"identity\": {"
                  << "\"real_uid\": " << id.real_uid << ","
                  << "\"effective_uid\": " << id.effective_uid << ","
                  << "\"real_gid\": " << id.real_gid << ","
                  << "\"effective_gid\": " << id.effective_gid << ","
                  << "\"is_root\": " << (id.is_root ? "true" : "false")
                  << "}\n";
    } else {
        std::cout << "\nIdentity\n";
        std::cout << "Real UID:      " << id.real_uid << '\n';
        std::cout << "Effective UID: " << id.effective_uid << '\n';
        std::cout << "Real GID:      " << id.real_gid << '\n';
        std::cout << "Effective GID: " << id.effective_gid << '\n';
        std::cout << "Is root:       " << (id.is_root ? "yes" : "no") << '\n';
    }
}

int main(int argc, char **argv) {
    bool json = false;
    for (int i = 1; i < argc; ++i) {
        std::string a{argv[i]};
        if (a == "--json" || a == "-j") json = true;
        if (a == "--help" || a == "-h") {
            std::cout << "Usage: example [--json|-j]\n";
            return 0;
        }
    }

    if (json) std::cout << "{\n";

    print_core(json);
    if (json) std::cout << ",\n";
    print_system(json);
    if (json) std::cout << ",\n";
    print_processes(json);
    if (json) std::cout << ",\n";
    print_disk_usage(json, "/");
    if (json) std::cout << ",\n";
    print_network(json);
    if (json) std::cout << ",\n";
    print_identity(json);

    if (json) std::cout << "}\n";

    return 0;
}