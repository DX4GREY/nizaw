#include "nizaw/core/error.hpp"

#include <cerrno>
#include <cstring>

namespace nizaw {

std::string_view to_string(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::Unknown:
            return "Unknown";
        case ErrorCode::NotFound:
            return "NotFound";
        case ErrorCode::PermissionDenied:
            return "PermissionDenied";
        case ErrorCode::InvalidArgument:
            return "InvalidArgument";
        case ErrorCode::Unsupported:
            return "Unsupported";
        case ErrorCode::IoError:
            return "IoError";
        case ErrorCode::ParseError:
            return "ParseError";
        case ErrorCode::ResourceUnavailable:
            return "ResourceUnavailable";
        case ErrorCode::AlreadyExists:
            return "AlreadyExists";
        case ErrorCode::CapabilityRequired:
            return "CapabilityRequired";
        case ErrorCode::OperationNotPermitted:
            return "OperationNotPermitted";
        case ErrorCode::ResourceBusy:
            return "ResourceBusy";
        case ErrorCode::WouldBlock:
            return "WouldBlock";
        case ErrorCode::InvalidState:
            return "InvalidState";
        case ErrorCode::PartialFailure:
            return "PartialFailure";
        case ErrorCode::ConfirmationRequired:
            return "ConfirmationRequired";
    }
    return "Unknown";
}

Error Error::from_errno(int errno_value, ErrorCode code, std::string_view source,
                         std::optional<std::string> message) {
    const auto detail = message.value_or(std::strerror(errno_value));
    return Error(code, std::string(detail), source, errno_value);
}

}  // namespace nizaw
