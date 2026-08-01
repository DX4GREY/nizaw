#include "nizaw/result.hpp"

#include <iostream>

int run_core_result_tests() {
    nizaw::Result<int> ok(42);
    if (!ok || ok.value() != 42) {
        std::cerr << "Result<int> value test failed" << std::endl;
        return 1;
    }

    nizaw::Result<int> failure(nizaw::Error(nizaw::ErrorCode::NotFound, "missing", "core"));
    if (failure) {
        std::cerr << "Result<int> error test unexpectedly succeeded" << std::endl;
        return 1;
    }

    if (failure.error().code() != nizaw::ErrorCode::NotFound) {
        std::cerr << "Result<int> error code test failed" << std::endl;
        return 1;
    }

    return 0;
}
