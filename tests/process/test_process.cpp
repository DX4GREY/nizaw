#include "nizaw/process.hpp"

#include <algorithm>
#include <iostream>
#include <unistd.h>

int run_process_tests() {
    auto list_result = nizaw::process::list();
    if (!list_result) {
        std::cerr << "Process list failed: " << list_result.error().message() << std::endl;
        return 1;
    }

    const auto& processes = list_result.value();
    if (processes.empty()) {
        std::cerr << "Process list was empty" << std::endl;
        return 1;
    }

    const auto self = std::find_if(processes.begin(), processes.end(), [](const auto& process) {
        return process.pid == getpid();
    });
    if (self == processes.end()) {
        std::cerr << "Current process was not found in process list" << std::endl;
        return 1;
    }

    auto inspect_result = nizaw::process::inspect(getpid());
    if (!inspect_result) {
        std::cerr << "Process inspect failed: " << inspect_result.error().message() << std::endl;
        return 1;
    }

    const auto& process = inspect_result.value();
    if (process.pid != getpid()) {
        std::cerr << "Process inspect returned unexpected PID" << std::endl;
        return 1;
    }

    return 0;
}
