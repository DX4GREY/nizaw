#include "nizaw/security.hpp"

#include <iostream>
#include <unistd.h>

int run_security_tests() {
    const auto identity_result = nizaw::security::identity();
    if (!identity_result) {
        std::cerr << "Security identity failed: " << identity_result.error().message() << std::endl;
        return 1;
    }

    const auto& identity = identity_result.value();
    if (identity.real_uid != getuid()) {
        std::cerr << "Real UID mismatch" << std::endl;
        return 1;
    }
    if (identity.effective_uid != geteuid()) {
        std::cerr << "Effective UID mismatch" << std::endl;
        return 1;
    }
    if (identity.real_gid != getgid()) {
        std::cerr << "Real GID mismatch" << std::endl;
        return 1;
    }
    if (identity.effective_gid != getegid()) {
        std::cerr << "Effective GID mismatch" << std::endl;
        return 1;
    }

    const auto capabilities_result = nizaw::security::capabilities();
    if (!capabilities_result) {
        std::cerr << "Security capabilities failed: " << capabilities_result.error().message() << std::endl;
        return 1;
    }

    // It's acceptable for a process to have no effective capabilities.
    for (const auto& capability : *capabilities_result) {
        if (capability.empty()) {
            std::cerr << "Empty capability name returned" << std::endl;
            return 1;
        }
    }

    return 0;
}
