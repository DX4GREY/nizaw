#include "nizaw/network.hpp"

#include <arpa/inet.h>
#include <ifaddrs.h>
#include <linux/if_packet.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>

namespace nizaw::network {
namespace {

std::string hex_to_ip(const std::string& hex) {
    if (hex.size() != 8) {
        return {};
    }
    
    unsigned int parts[4];
    std::sscanf(hex.c_str(), "%2x%2x%2x%2x", &parts[0], &parts[1], &parts[2], &parts[3]);
    
    char buf[INET_ADDRSTRLEN];
    struct in_addr addr;
    addr.s_addr = (parts[0] << 24) | (parts[1] << 16) | (parts[2] << 8) | parts[3];
    
    if (inet_ntop(AF_INET, &addr, buf, sizeof(buf)) == nullptr) {
        return {};
    }
    return buf;
}

std::string hex_to_ipv6(const std::string& hex) {
    if (hex.size() != 32) {
        return {};
    }
    
    unsigned int parts[4];
    std::sscanf(hex.c_str(), "%8x%8x%8x%8x", &parts[0], &parts[1], &parts[2], &parts[3]);
    
    char buf[INET6_ADDRSTRLEN];
    struct in6_addr addr;
    addr.s6_addr32[0] = htonl(parts[0]);
    addr.s6_addr32[1] = htonl(parts[1]);
    addr.s6_addr32[2] = htonl(parts[2]);
    addr.s6_addr32[3] = htonl(parts[3]);
    
    if (inet_ntop(AF_INET6, &addr, buf, sizeof(buf)) == nullptr) {
        return {};
    }
    return buf;
}

std::uint16_t hex_to_port(const std::string& hex) {
    return static_cast<std::uint16_t>(std::stoul(hex, nullptr, 16));
}

std::string tcp_state(int state) {
    switch (state) {
        case 1: return "ESTABLISHED";
        case 2: return "SYN_SENT";
        case 3: return "SYN_RECV";
        case 4: return "FIN_WAIT1";
        case 5: return "FIN_WAIT2";
        case 6: return "TIME_WAIT";
        case 7: return "CLOSE";
        case 8: return "CLOSE_WAIT";
        case 9: return "LAST_ACK";
        case 10: return "LISTEN";
        case 11: return "CLOSING";
        default: return "UNKNOWN";
    }
}

Result<std::vector<ConnectionInfo>> parse_proc_net(const std::string& filepath, const std::string& protocol) {
    std::ifstream file(filepath);
    if (!file) {
        return Error(ErrorCode::NotFound, filepath + " is unavailable", "network");
    }
    
    std::vector<ConnectionInfo> connections;
    std::string line;
    
    // Skip header line
    std::getline(file, line);
    
    while (std::getline(file, line)) {
        if (line.empty()) {
            continue;
        }
        
        std::istringstream stream(line);
        ConnectionInfo conn;
        conn.protocol = protocol;
        
        // Read sequence number with colon (e.g., "0:")
        std::string seq;
        stream >> seq;
        
        // Read local address:port and remote address:port
        std::string local_full, remote_full;
        stream >> local_full >> remote_full;
        
        // Parse state
        std::string state_hex;
        stream >> state_hex;
        
        // Read remaining fields
        // tx_queue:rx_queue is ONE field (e.g., "00000000:00000000")
        std::string tx_rx_queues;
        stream >> tx_rx_queues;
        
        // tr:tm->when is ONE field (e.g., "00:00000000")
        std::string tr_tm_when;
        stream >> tr_tm_when;
        
        // retrnsmt
        std::string retrnsmt;
        stream >> retrnsmt;
        
        // uid timeout inode
        unsigned int uid = 0, timeout = 0;
        unsigned long inode = 0;
        stream >> uid >> timeout >> inode;
        
        // Split address:port
        auto split_addr_port = [](const std::string& full, std::string& addr, std::string& port) {
            const auto pos = full.find(':');
            if (pos != std::string::npos) {
                addr = full.substr(0, pos);
                port = full.substr(pos + 1);
            }
        };
        
        std::string local_addr_hex, local_port_hex;
        std::string remote_addr_hex, remote_port_hex;
        split_addr_port(local_full, local_addr_hex, local_port_hex);
        split_addr_port(remote_full, remote_addr_hex, remote_port_hex);
        
        // Convert hex to readable format
        if (protocol == "tcp6" || protocol == "udp6") {
            conn.local_address = hex_to_ipv6(local_addr_hex);
            conn.remote_address = hex_to_ipv6(remote_addr_hex);
        } else {
            conn.local_address = hex_to_ip(local_addr_hex);
            conn.remote_address = hex_to_ip(remote_addr_hex);
        }
        
        conn.local_port = hex_to_port(local_port_hex);
        conn.remote_port = hex_to_port(remote_port_hex);
        conn.state = tcp_state(static_cast<int>(std::stoul(state_hex, nullptr, 16)));
        conn.uid = static_cast<std::uint32_t>(uid);
        conn.inode = static_cast<std::uint32_t>(inode);
        
        connections.push_back(conn);
    }
    
    return connections;
}


std::string to_hex_mac(const unsigned char* data, std::size_t length) {
    std::ostringstream out;
    out << std::hex << std::setfill('0');
    for (std::size_t i = 0; i < length; ++i) {
        out << std::setw(2) << static_cast<int>(data[i]);
        if (i + 1 < length) {
            out << ':';
        }
    }
    return out.str();
}

std::string address_to_string(const sockaddr* addr) {
    if (addr == nullptr) {
        return {};
    }
    char buf[INET6_ADDRSTRLEN] = {};
    if (addr->sa_family == AF_INET) {
        const auto* v4 = reinterpret_cast<const sockaddr_in*>(addr);
        if (inet_ntop(AF_INET, &v4->sin_addr, buf, sizeof(buf)) == nullptr) {
            return {};
        }
        return buf;
    }
    if (addr->sa_family == AF_INET6) {
        const auto* v6 = reinterpret_cast<const sockaddr_in6*>(addr);
        if (inet_ntop(AF_INET6, &v6->sin6_addr, buf, sizeof(buf)) == nullptr) {
            return {};
        }
        return buf;
    }
    return {};
}

std::vector<std::string> flags_from_mask(unsigned flags) {
    std::vector<std::string> result;
    if (flags & IFF_UP) result.push_back("UP");
    if (flags & IFF_BROADCAST) result.push_back("BROADCAST");
    if (flags & IFF_LOOPBACK) result.push_back("LOOPBACK");
    if (flags & IFF_POINTOPOINT) result.push_back("POINTOPOINT");
    if (flags & IFF_RUNNING) result.push_back("RUNNING");
    if (flags & IFF_MULTICAST) result.push_back("MULTICAST");
    if (flags & IFF_PROMISC) result.push_back("PROMISC");
    if (flags & IFF_MASTER) result.push_back("MASTER");
    if (flags & IFF_SLAVE) result.push_back("SLAVE");
    return result;
}

std::string operstate(const std::string& name) {
    const std::filesystem::path state_path = std::filesystem::path("/sys/class/net") / name / "operstate";
    std::ifstream input(state_path);
    if (!input) {
        return "unknown";
    }
    std::string value;
    std::getline(input, value);
    return value.empty() ? "unknown" : value;
}

std::uint64_t read_stat(const std::string& name, const std::string& stat_name) {
    const std::filesystem::path stat_path = std::filesystem::path("/sys/class/net") / name / "statistics" / stat_name;
    std::ifstream input(stat_path);
    if (!input) {
        return 0;
    }
    std::uint64_t value = 0;
    input >> value;
    return value;
}

void fill_interface_statistics(InterfaceInfo& info) {
    info.rx_bytes = read_stat(info.name, "rx_bytes");
    info.tx_bytes = read_stat(info.name, "tx_bytes");
    info.rx_packets = read_stat(info.name, "rx_packets");
    info.tx_packets = read_stat(info.name, "tx_packets");
    info.rx_errors = read_stat(info.name, "rx_errors");
    info.tx_errors = read_stat(info.name, "tx_errors");
    info.rx_dropped = read_stat(info.name, "rx_dropped");
    info.tx_dropped = read_stat(info.name, "tx_dropped");
}

Result<void> fill_interface_details(InterfaceInfo& info) {
    const int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) {
        return Error::from_errno(errno, ErrorCode::IoError, "network", "socket(AF_INET, SOCK_DGRAM) failed");
    }

    ifreq request{};
    std::strncpy(request.ifr_name, info.name.c_str(), IFNAMSIZ - 1);

    if (ioctl(fd, SIOCGIFMTU, &request) == 0) {
        info.mtu = request.ifr_mtu;
    }

    if (ioctl(fd, SIOCGIFFLAGS, &request) == 0) {
        info.flags = flags_from_mask(request.ifr_flags);
    }

    if (ioctl(fd, SIOCGIFHWADDR, &request) == 0) {
        const auto* hw = reinterpret_cast<unsigned char*>(request.ifr_hwaddr.sa_data);
        info.mac_address = to_hex_mac(hw, 6);
    }

    close(fd);
    return {};
}

}  // namespace

Result<std::vector<InterfaceInfo>> list() {
    struct ifaddrs* ifaddr = nullptr;
    if (getifaddrs(&ifaddr) != 0) {
        return Error::from_errno(errno, ErrorCode::IoError, "network", "getifaddrs() failed");
    }
    std::unique_ptr<ifaddrs, decltype(&freeifaddrs)> cleanup(ifaddr, freeifaddrs);

    std::map<std::string, InterfaceInfo> interfaces;
    for (struct ifaddrs* ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next) {
        if (ifa->ifa_name == nullptr) {
            continue;
        }
        auto& iface = interfaces[ifa->ifa_name];
        iface.name = ifa->ifa_name;
        iface.index = if_nametoindex(ifa->ifa_name);
        iface.state = operstate(iface.name);

        if (ifa->ifa_addr != nullptr) {
            if (ifa->ifa_addr->sa_family == AF_INET || ifa->ifa_addr->sa_family == AF_INET6) {
                InterfaceAddress address;
                address.family = ifa->ifa_addr->sa_family == AF_INET ? "inet" : "inet6";
                address.address = address_to_string(ifa->ifa_addr);
                address.netmask = address_to_string(ifa->ifa_netmask);
                if (ifa->ifa_broadaddr != nullptr) {
                    address.broadcast = address_to_string(ifa->ifa_broadaddr);
                }
                iface.addresses.push_back(std::move(address));
            }
        }
    }

    std::vector<InterfaceInfo> result;
    for (auto& [name, iface] : interfaces) {
        if (auto error = fill_interface_details(iface); error) {
            continue;
        }
        fill_interface_statistics(iface);
        result.push_back(std::move(iface));
    }

    return result;
}

Result<InterfaceInfo> inspect(std::string_view interface_name) {
    const auto all = list();
    if (!all) {
        return all.error();
    }

    for (auto& iface : all.value()) {
        if (iface.name == interface_name) {
            return iface;
        }
    }
    return Error(ErrorCode::NotFound, "Interface not found", "network");
}

Result<std::vector<ConnectionInfo>> connections() {
    std::vector<ConnectionInfo> all_connections;
    
    // Parse TCP connections
    auto tcp_result = parse_proc_net("/proc/net/tcp", "tcp");
    if (tcp_result) {
        all_connections.insert(all_connections.end(), 
                              std::make_move_iterator(tcp_result.value().begin()),
                              std::make_move_iterator(tcp_result.value().end()));
    }
    
    // Parse TCP6 connections
    auto tcp6_result = parse_proc_net("/proc/net/tcp6", "tcp6");
    if (tcp6_result) {
        all_connections.insert(all_connections.end(),
                              std::make_move_iterator(tcp6_result.value().begin()),
                              std::make_move_iterator(tcp6_result.value().end()));
    }
    
    // Parse UDP connections
    auto udp_result = parse_proc_net("/proc/net/udp", "udp");
    if (udp_result) {
        all_connections.insert(all_connections.end(),
                              std::make_move_iterator(udp_result.value().begin()),
                              std::make_move_iterator(udp_result.value().end()));
    }
    
    // Parse UDP6 connections
    auto udp6_result = parse_proc_net("/proc/net/udp6", "udp6");
    if (udp6_result) {
        all_connections.insert(all_connections.end(),
                              std::make_move_iterator(udp6_result.value().begin()),
                              std::make_move_iterator(udp6_result.value().end()));
    }
    
    return all_connections;
}

}  // namespace nizaw::network
