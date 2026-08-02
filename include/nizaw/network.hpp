#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "nizaw/result.hpp"

namespace nizaw::network {

struct InterfaceAddress {
    std::string family;
    std::string address;
    std::string netmask;
    std::string broadcast;
};

struct InterfaceInfo {
    std::string name;
    unsigned index = 0;
    std::string state;
    std::string mac_address;
    int mtu = 0;
    std::vector<std::string> flags;
    std::vector<InterfaceAddress> addresses;
};

struct ConnectionInfo {
    std::string protocol;
    std::string local_address;
    std::uint16_t local_port = 0;
    std::string remote_address;
    std::uint16_t remote_port = 0;
    std::string state;
    std::uint32_t uid = 0;
    std::uint32_t inode = 0;
};

[[nodiscard]] Result<std::vector<InterfaceInfo>> list();
[[nodiscard]] Result<InterfaceInfo> inspect(std::string_view interface_name);
[[nodiscard]] Result<std::vector<ConnectionInfo>> connections();

}  // namespace nizaw::network
