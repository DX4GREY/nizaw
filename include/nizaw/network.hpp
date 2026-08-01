#pragma once

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

[[nodiscard]] Result<std::vector<InterfaceInfo>> list();
[[nodiscard]] Result<InterfaceInfo> inspect(std::string_view interface_name);

}  // namespace nizaw::network
