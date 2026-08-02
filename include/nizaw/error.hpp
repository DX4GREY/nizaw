#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace nizaw {

/// Stable, module-agnostic classification of what went wrong.
///
/// This enum is part of the public API surface: once Nizaw reaches v1.0.0,
/// values are never renumbered or removed (only appended to), so callers can
/// safely switch on `ErrorCode` across library upgrades.
enum class ErrorCode {
    Unknown = 0,
    NotFound,
    PermissionDenied,
    InvalidArgument,
    Unsupported,
    IoError,
    ParseError,
    ResourceUnavailable,
    AlreadyExists,
    CapabilityRequired,
    OperationNotPermitted,
    ResourceBusy,
    WouldBlock,
    InvalidState,
    PartialFailure,
    ConfirmationRequired,
};

/// Human-readable name for an ErrorCode, e.g. for logging or JSON output.
/// Never throws; unknown/out-of-range values map to "Unknown".
[[nodiscard]] std::string_view to_string(ErrorCode code) noexcept;

/// Carries everything a caller needs to understand and react to a failure:
/// what kind of failure it was (`code`), a human-readable explanation
/// (`message`), which module raised it (`source`), and — if the failure
/// originated from a syscall — the raw `errno` value.
///
/// `Error` is a plain value type: copyable, movable, comparable only by
/// inspecting its fields (no operator== is provided, since "equal" isn't a
/// well-defined concept for free-text messages).
class Error {
public:
    Error(ErrorCode code, std::string message, std::string_view source,
          std::optional<int> errno_value = std::nullopt)
        : code_(code),
          message_(std::move(message)),
          source_(source),
          errno_value_(errno_value) {}

    [[nodiscard]] ErrorCode code() const noexcept { return code_; }
    [[nodiscard]] const std::string& message() const noexcept { return message_; }
    [[nodiscard]] const std::string& source() const noexcept { return source_; }
    [[nodiscard]] std::optional<int> errno_value() const noexcept { return errno_value_; }

    /// Convenience factory: build an Error from the current `errno`, using
    /// `strerror` for the message unless an explicit message is supplied.
    /// `errno` is captured eagerly by the caller (before this call), since by
    /// the time this function's body runs `errno` may already have been
    /// clobbered by other library calls (e.g. inside message formatting).
    static Error from_errno(int errno_value, ErrorCode code, std::string_view source,
                             std::optional<std::string> message = std::nullopt);

private:
    ErrorCode code_;
    std::string message_;
    std::string source_;
    std::optional<int> errno_value_;
};

}  // namespace nizaw
