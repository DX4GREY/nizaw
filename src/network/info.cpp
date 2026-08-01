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
            result.push_back(std::move(iface));
        }
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

}  // namespace nizaw::network
