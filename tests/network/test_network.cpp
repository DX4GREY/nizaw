#include "nizaw/network.hpp"

#include <iostream>

int run_network_tests() {
    const auto interfaces = nizaw::network::list();
    if (!interfaces) {
        std::cerr << "Network enumeration failed: " << interfaces.error().message() << std::endl;
        return 1;
    }

    // In containerized/restricted environments, no interfaces may be available
    if (interfaces->empty()) {
        std::cout << "Network enumeration returned zero interfaces (likely containerized environment)" << std::endl;
        return 0;
    }

    const auto& first = interfaces->front();
    if (first.name.empty()) {
        std::cerr << "First interface name was empty" << std::endl;
        return 1;
    }

    const auto inspect_result = nizaw::network::inspect(first.name);
    if (!inspect_result) {
        std::cerr << "Network inspect failed: " << inspect_result.error().message() << std::endl;
        return 1;
    }

    const auto& inspected = inspect_result.value();
    if (inspected.name != first.name) {
        std::cerr << "Network inspect returned a different interface" << std::endl;
        return 1;
    }

    return 0;
}
