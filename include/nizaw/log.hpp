#pragma once

#include <format>
#include <mutex>
#include <string_view>

namespace nizaw::core {

/// Log severity, ordered so `level >= threshold` means "should be emitted".
/// `Off` is a threshold-only value (never actually logged at) that disables
/// all output.
enum class LogLevel { Trace = 0, Debug, Info, Warn, Error, Fatal, Off };

[[nodiscard]] std::string_view to_string(LogLevel level) noexcept;

/// A minimal, dependency-free logging sink. Deliberately does not pull in a
/// third-party logging library (see docs/dependency-policy.md §6) — Nizaw's
/// needs (a handful of levels, a single text sink, level filtering) don't
/// justify the added dependency.
///
/// Thread-safety: `set_level`/`level` are safe to call concurrently with
/// logging calls; the process-wide singleton instance itself is guarded by
/// a mutex around the actual write so interleaved multi-threaded log lines
/// don't get garbled mid-line. This is NOT a high-throughput structured
/// logger — it is a diagnostic/observability aid, matching Nizaw's
/// synchronous-by-default threading model (see docs/architecture.md §10).
class Logger {
public:
    static Logger& instance() noexcept;

    void set_level(LogLevel level) noexcept;
    [[nodiscard]] LogLevel level() const noexcept;

    /// Writes `message` at `level` to stderr (all levels — stdout is
    /// reserved for command output/JSON, never mixed with diagnostics) if
    /// `level` is at or above the current threshold. `source` identifies
    /// the emitting module, e.g. "storage".
    void write(LogLevel level, std::string_view source, std::string_view message);

private:
    Logger() = default;

    LogLevel level_ = LogLevel::Info;
    mutable std::mutex mutex_;
};

/// Formats `fmt`/`args` with `std::format` and writes it at `level` if the
/// current threshold allows — the format/allocation cost is only paid when
/// the message would actually be emitted.
template <typename... Args>
void log(LogLevel level, std::string_view source, std::format_string<Args...> fmt,
         Args&&... args) {
    auto& logger = Logger::instance();
    if (level < logger.level()) {
        return;
    }
    logger.write(level, source, std::format(fmt, std::forward<Args>(args)...));
}

}  // namespace nizaw::core

/// `source` is a short module tag, e.g. "storage", "process" — matches the
/// `Error::source()` convention so log lines and errors can be correlated.
#define NIZAW_LOG_TRACE(source, ...) \
    ::nizaw::core::log(::nizaw::core::LogLevel::Trace, source, __VA_ARGS__)
#define NIZAW_LOG_DEBUG(source, ...) \
    ::nizaw::core::log(::nizaw::core::LogLevel::Debug, source, __VA_ARGS__)
#define NIZAW_LOG_INFO(source, ...) \
    ::nizaw::core::log(::nizaw::core::LogLevel::Info, source, __VA_ARGS__)
#define NIZAW_LOG_WARN(source, ...) \
    ::nizaw::core::log(::nizaw::core::LogLevel::Warn, source, __VA_ARGS__)
#define NIZAW_LOG_ERROR(source, ...) \
    ::nizaw::core::log(::nizaw::core::LogLevel::Error, source, __VA_ARGS__)
#define NIZAW_LOG_FATAL(source, ...) \
    ::nizaw::core::log(::nizaw::core::LogLevel::Fatal, source, __VA_ARGS__)
