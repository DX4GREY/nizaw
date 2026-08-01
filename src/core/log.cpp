#include "nizaw/core/log.hpp"

#include <iostream>

namespace nizaw::core {

std::string_view to_string(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::Trace:
            return "TRACE";
        case LogLevel::Debug:
            return "DEBUG";
        case LogLevel::Info:
            return "INFO";
        case LogLevel::Warn:
            return "WARN";
        case LogLevel::Error:
            return "ERROR";
        case LogLevel::Fatal:
            return "FATAL";
        case LogLevel::Off:
            return "OFF";
    }
    return "INFO";
}

Logger& Logger::instance() noexcept {
    static Logger logger;
    return logger;
}

void Logger::set_level(LogLevel level) noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    level_ = level;
}

LogLevel Logger::level() const noexcept {
    std::lock_guard<std::mutex> lock(mutex_);
    return level_;
}

void Logger::write(LogLevel level, std::string_view source, std::string_view message) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (level < level_) {
        return;
    }
    std::cerr << '[' << to_string(level) << "] " << source << ": " << message << '\n';
}

}  // namespace nizaw::core
