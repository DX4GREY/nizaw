#include "nizaw/storage.hpp"

#include <iostream>

int run_storage_tests() {
    const auto devices = nizaw::storage::enumerate();
    if (!devices) {
        std::cerr << "Storage enumeration failed: " << devices.error().message() << std::endl;
        return 1;
    }

    if (devices->empty()) {
        std::cerr << "Storage enumeration returned zero devices" << std::endl;
        return 1;
    }

    const auto& device = devices->front();
    if (device.name.empty()) {
        std::cerr << "Storage device name was empty" << std::endl;
        return 1;
    }

    return 0;
}
