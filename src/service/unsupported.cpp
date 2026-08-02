#include "nizaw/service.hpp"

namespace nizaw::service {

Result<std::vector<ServiceInfo>> list() {
    return Error(ErrorCode::Unsupported, "Systemd support unavailable", "service");
}

Result<ServiceInfo> inspect(std::string_view) {
    return Error(ErrorCode::Unsupported, "Systemd support unavailable", "service");
}

Result<void> control(std::string_view, ServiceAction, const core::WriteOptions&) {
    return Error(ErrorCode::Unsupported, "Systemd support unavailable", "service");
}

Result<void> enable(std::string_view, const core::WriteOptions&) {
    return Error(ErrorCode::Unsupported, "Systemd support unavailable", "service");
}

Result<void> disable(std::string_view, const core::WriteOptions&) {
    return Error(ErrorCode::Unsupported, "Systemd support unavailable", "service");
}

}  // namespace nizaw::service
