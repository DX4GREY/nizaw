#include "nizaw/service.hpp"

namespace nizaw::service {

Result<std::vector<ServiceInfo>> list() {
    return Error(ErrorCode::Unsupported, "Systemd support unavailable", "service");
}

Result<ServiceInfo> inspect(std::string_view) {
    return Error(ErrorCode::Unsupported, "Systemd support unavailable", "service");
}

}  // namespace nizaw::service
