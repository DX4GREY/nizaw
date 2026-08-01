#include "nizaw/core/platform.hpp"

#include <filesystem>
#include <fstream>
#include <string>

namespace nizaw::core {

PlatformInfo detect() noexcept {
    PlatformInfo info;

    std::ifstream os_release("/etc/os-release");
    if (!os_release) {
        return info;
    }

    std::string line;
    while (std::getline(os_release, line)) {
        const auto separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        std::string key = line.substr(0, separator);
        std::string value = line.substr(separator + 1);
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        }

        if (key == "ID") {
            info.distro_id = value;
        } else if (key == "NAME") {
            info.distro_name = value;
        } else if (key == "VERSION_ID") {
            info.distro_version = value;
        } else if (key == "VERSION") {
            if (info.distro_name.empty()) {
                info.distro_name = value;
            }
        }
    }

    if (info.distro_name.empty() && !info.distro_id.empty()) {
        info.distro_name = info.distro_id;
    }

    info.has_systemd = std::filesystem::exists("/run/systemd/system");
    return info;
}

}  // namespace nizaw::core
