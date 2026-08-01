#include "nizaw/service.hpp"

#include <iostream>

int run_service_tests() {
    const auto services = nizaw::service::list();
    if (!services) {
        if (services.error().code() == nizaw::ErrorCode::Unsupported) {
            return 0;
        }

        std::cerr << "Service list failed: " << services.error().message() << std::endl;
        return 1;
    }

    if (services->empty()) {
        std::cerr << "Service list returned zero services" << std::endl;
        return 1;
    }

    const auto& first = services->front();
    if (first.name.empty()) {
        std::cerr << "First service name was empty" << std::endl;
        return 1;
    }

    const auto inspect_result = nizaw::service::inspect(first.name);
    if (!inspect_result) {
        std::cerr << "Service inspect failed: " << inspect_result.error().message() << std::endl;
        return 1;
    }

    if (inspect_result->name != first.name) {
        std::cerr << "Service inspect returned a different service" << std::endl;
        return 1;
    }

    return 0;
}
