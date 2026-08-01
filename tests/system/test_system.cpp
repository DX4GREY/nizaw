#include "nizaw/system.hpp"

#include <iostream>

int run_system_info_tests() {
    const auto info = nizaw::system::info();
    if (!info) {
        std::cerr << "System info query failed: " << info.error().message() << std::endl;
        return 1;
    }

    const auto& system_info = info.value();
    if (system_info.hostname.empty()) {
        std::cerr << "System hostname was empty" << std::endl;
        return 1;
    }
    if (system_info.kernel_name.empty()) {
        std::cerr << "Kernel name was empty" << std::endl;
        return 1;
    }
    if (system_info.page_size == 0) {
        std::cerr << "Page size was zero" << std::endl;
        return 1;
    }

    return 0;
}
