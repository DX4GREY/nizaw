#include "nizaw/core/log.hpp"

#include <iostream>

int run_core_log_tests() {
    auto& logger = nizaw::core::Logger::instance();
    logger.set_level(nizaw::core::LogLevel::Trace);
    if (logger.level() != nizaw::core::LogLevel::Trace) {
        std::cerr << "Logger level change failed" << std::endl;
        return 1;
    }
    NIZAW_LOG_INFO("core", "logger smoke test");
    return 0;
}
