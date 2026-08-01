#include "nizaw/core/env.hpp"

#include <cstdlib>
#include <iostream>
#include <string>

int run_core_env_tests() {
    setenv("NIZAW_TEST_VALUE", "hello", 1);
    auto value = nizaw::core::env::get("NIZAW_TEST_VALUE");
    if (!value || *value != "hello") {
        std::cerr << "Environment get test failed" << std::endl;
        return 1;
    }

    if (!nizaw::core::env::exists("NIZAW_TEST_VALUE")) {
        std::cerr << "Environment exists test failed" << std::endl;
        return 1;
    }

    return 0;
}
