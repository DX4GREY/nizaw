#include "nizaw/core/platform.hpp"

#include <iostream>

int run_core_platform_tests() {
    const auto info = nizaw::core::detect();
    if (info.distro_id.empty() && info.distro_name.empty() && info.distro_version.empty()) {
        std::cerr << "Platform detection returned empty identifiers" << std::endl;
        return 1;
    }
    return 0;
}
