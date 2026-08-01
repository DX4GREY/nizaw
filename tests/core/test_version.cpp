#include "nizaw/core/version.hpp"

#include <iostream>

int run_core_version_tests() {
    const auto version = nizaw::core::version();
    if (version.major == 0 && version.minor == 0 && version.patch == 0) {
        std::cout << "Version metadata is at default value" << std::endl;
    }

    const auto string_value = nizaw::core::version_string();
    if (string_value.empty()) {
        std::cerr << "Version string should be non-empty" << std::endl;
        return 1;
    }
    return 0;
}
