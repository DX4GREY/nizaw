#include "nizaw/service.hpp"

#include "nizaw/core/log.hpp"
#include "nizaw/core/write.hpp"

#include <systemd/sd-bus.h>

#include <cerrno>
#include <cstring>
#include <optional>
#include <string>
#include <vector>

namespace nizaw::service {
namespace {

std::string to_string_or_unknown(const char* value) {
    return value ? std::string(value) : std::string("unknown");
}

std::string bus_error_message(int r) {
    if (r >= 0) {
        return std::string("OK");
    }
    const char* err = strerror(-r);
    return err ? std::string(err) : std::string("unknown bus error");
}

Result<sd_bus*> open_system_bus() {
    sd_bus* bus = nullptr;
    const int r = sd_bus_open_system(&bus);
    if (r < 0) {
        return Error::from_errno(-r, ErrorCode::IoError, "service", "sd_bus_open_system() failed");
    }
    return bus;
}

std::optional<ServiceInfo> parse_unit_entry(const char* name, const char* description, const char* load_state,
                                             const char* active_state, const char* sub_state) {
    if (!name) {
        return std::nullopt;
    }
    const std::string unit_name = name;
    if (unit_name.size() < 8 || unit_name.substr(unit_name.size() - 8) != ".service") {
        return std::nullopt;
    }

    ServiceInfo info;
    info.name = unit_name;
    info.description = to_string_or_unknown(description);
    info.load_state = to_string_or_unknown(load_state);
    info.active_state = to_string_or_unknown(active_state);
    info.sub_state = to_string_or_unknown(sub_state);
    info.loaded = (info.load_state == "loaded");
    info.active = (info.active_state == "active");
    return info;
}

Result<std::vector<ServiceInfo>> list_units(sd_bus* bus) {
    sd_bus_message* reply = nullptr;
    int r = sd_bus_call_method(bus,
                                "org.freedesktop.systemd1",
                                "/org/freedesktop/systemd1",
                                "org.freedesktop.systemd1.Manager",
                                "ListUnits",
                                nullptr,
                                &reply,
                                "");
    if (r < 0) {
        if (reply) {
            sd_bus_message_unref(reply);
        }
        return Error::from_errno(-r, ErrorCode::IoError, "service", "ListUnits call failed");
    }

    std::vector<ServiceInfo> services;
    r = sd_bus_message_enter_container(reply, SD_BUS_TYPE_ARRAY, "(ssssssouso)");
    if (r < 0) {
        sd_bus_message_unref(reply);
        return Error::from_errno(-r, ErrorCode::ParseError, "service", "Failed to enter response array");
    }

    while ((r = sd_bus_message_enter_container(reply, SD_BUS_TYPE_STRUCT, "ssssssouso")) > 0) {
        const char* name = nullptr;
        const char* description = nullptr;
        const char* load_state = nullptr;
        const char* active_state = nullptr;
        const char* sub_state = nullptr;
        const char* following = nullptr;
        uint32_t job_id = 0;
        const char* job_type = nullptr;
        const char* job_path = nullptr;

        r = sd_bus_message_read(reply, "ssssssouso", &name, &description, &load_state, &active_state,
                                &sub_state, &following, &job_id, &job_type, &job_path);
        if (r < 0) {
            sd_bus_message_unref(reply);
            return Error::from_errno(-r, ErrorCode::ParseError, "service", "Failed to parse unit struct");
        }

        if (const auto maybe_info = parse_unit_entry(name, description, load_state, active_state, sub_state)) {
            services.push_back(*maybe_info);
        }

        sd_bus_message_exit_container(reply);
    }

    sd_bus_message_exit_container(reply);
    sd_bus_message_unref(reply);
    return services;
}

Result<std::string> get_unit_file_state(sd_bus* bus, const std::string& unit_name) {
    sd_bus_message* reply = nullptr;
    int r = sd_bus_call_method(bus,
                                "org.freedesktop.systemd1",
                                "/org/freedesktop/systemd1",
                                "org.freedesktop.systemd1.Manager",
                                "GetUnitFileState",
                                nullptr,
                                &reply,
                                "s",
                                unit_name.c_str());
    if (r < 0) {
        if (reply) {
            sd_bus_message_unref(reply);
        }
        return Error::from_errno(-r, ErrorCode::IoError, "service", "GetUnitFileState call failed");
    }

    const char* state = nullptr;
    r = sd_bus_message_read(reply, "s", &state);
    sd_bus_message_unref(reply);
    if (r < 0) {
        return Error::from_errno(-r, ErrorCode::ParseError, "service", "Failed to read unit file state");
    }
    return to_string_or_unknown(state);
}

// Note: sd_bus_get_property_uint64 may not be available in all libsystemd versions
// For now, we'll skip MainPID retrieval in inspect to maintain compatibility
Result<uint64_t> get_main_pid(sd_bus* /*bus*/, const std::string& /*unit_path*/) {
    // Implementation deferred - requires sd_bus_get_property or similar
    return 0;
}

Result<ServiceInfo> inspect_unit(sd_bus* bus, std::string_view unit_name) {
    const auto list_result = list_units(bus);
    if (!list_result) {
        return list_result.error();
    }

    for (auto service : list_result.value()) {
        if (service.name == unit_name) {
            const auto enabled_result = get_unit_file_state(bus, service.name);
            if (enabled_result) {
                service.enabled = (enabled_result.value() == "enabled");
            }
            // MainPID requires retrieving the unit object.
            sd_bus_message* unit_reply = nullptr;
            int r = sd_bus_call_method(bus,
                                        "org.freedesktop.systemd1",
                                        "/org/freedesktop/systemd1",
                                        "org.freedesktop.systemd1.Manager",
                                        "GetUnit",
                                        nullptr,
                                        &unit_reply,
                                        "s",
                                        service.name.c_str());
            if (r >= 0) {
                const char* unit_path = nullptr;
                if (sd_bus_message_read(unit_reply, "o", &unit_path) >= 0 && unit_path != nullptr) {
                    const auto pid_result = get_main_pid(bus, unit_path);
                    if (pid_result) {
                        service.main_pid = static_cast<pid_t>(pid_result.value());
                    }
                }
            }
            if (unit_reply) {
                sd_bus_message_unref(unit_reply);
            }
            return service;
        }
    }
    return Error(ErrorCode::NotFound, "Service not found", "service");
}

}  // namespace

struct BusHandle {
    sd_bus* bus = nullptr;
    ~BusHandle() {
        if (bus) {
            sd_bus_unref(bus);
        }
    }
};

Result<std::vector<ServiceInfo>> list() {
    const auto bus_result = open_system_bus();
    if (!bus_result) {
        return bus_result.error();
    }

    BusHandle handle{bus_result.value()};
    return list_units(handle.bus);
}

Result<ServiceInfo> inspect(std::string_view unit_name) {
    const auto bus_result = open_system_bus();
    if (!bus_result) {
        return bus_result.error();
    }

    BusHandle handle{bus_result.value()};
    return inspect_unit(handle.bus, unit_name);
}

// Write operations implementation

Result<void> control(std::string_view unit_name,
                     ServiceAction action,
                     const core::WriteOptions& options) {
    if (options.dry_run) {
        const char* action_name = "";
        switch (action) {
            case ServiceAction::Start:
                action_name = "start";
                break;
            case ServiceAction::Stop:
                action_name = "stop";
                break;
            case ServiceAction::Restart:
                action_name = "restart";
                break;
            case ServiceAction::Reload:
                action_name = "reload";
                break;
        }
        std::string msg = "Would " + std::string(action_name) + " service '" + std::string(unit_name) + "'";
        nizaw::core::Logger::instance().write(nizaw::core::LogLevel::Info, "service", msg);
        return {};
    }

    if (options.confirm_prompt) {
        nizaw::core::Logger::instance().write(nizaw::core::LogLevel::Warn, "service", "Confirmation required: " + *options.confirm_prompt);
        return Error(ErrorCode::ConfirmationRequired, 
                     "Confirmation required for service control", "service");
    }

    const auto bus_result = open_system_bus();
    if (!bus_result) {
        return bus_result.error();
    }

    BusHandle handle{bus_result.value()};
    sd_bus* bus = handle.bus;

    const char* method_name = nullptr;
    switch (action) {
        case ServiceAction::Start:
            method_name = "StartUnit";
            break;
        case ServiceAction::Stop:
            method_name = "StopUnit";
            break;
        case ServiceAction::Restart:
            method_name = "RestartUnit";
            break;
        case ServiceAction::Reload:
            method_name = "ReloadUnit";
            break;
    }

    sd_bus_message* reply = nullptr;
    int r = sd_bus_call_method(bus,
                                "org.freedesktop.systemd1",
                                "/org/freedesktop/systemd1",
                                "org.freedesktop.systemd1.Manager",
                                method_name,
                                nullptr,
                                &reply,
                                "ss",
                                std::string(unit_name).c_str(),
                                "replace");
    if (r < 0) {
        if (reply) {
            sd_bus_message_unref(reply);
        }
        return Error::from_errno(-r, ErrorCode::IoError, "service", 
                                 std::string(method_name) + " call failed");
    }

    sd_bus_message_unref(reply);

    const char* action_name = "";
    switch (action) {
        case ServiceAction::Start:
            action_name = "started";
            break;
        case ServiceAction::Stop:
            action_name = "stopped";
            break;
        case ServiceAction::Restart:
            action_name = "restarted";
            break;
        case ServiceAction::Reload:
            action_name = "reloaded";
            break;
    }

    std::string action_msg = std::string(action_name) + " service '" + std::string(unit_name) + "'";
    nizaw::core::Logger::instance().write(nizaw::core::LogLevel::Info, "service", action_msg);
    core::AuditLogger::instance().log("service", 
                                      std::string(action_name), 
                                      std::string(unit_name), true);
    return {};
}

Result<void> enable(std::string_view unit_name, const core::WriteOptions& options) {
    if (options.dry_run) {
        std::string msg = "Would enable service '" + std::string(unit_name) + "'";
        nizaw::core::Logger::instance().write(nizaw::core::LogLevel::Info, "service", msg);
        return {};
    }

    if (options.confirm_prompt) {
        nizaw::core::Logger::instance().write(nizaw::core::LogLevel::Warn, "service", "Confirmation required: " + *options.confirm_prompt);
        return Error(ErrorCode::ConfirmationRequired, 
                     "Confirmation required for enable", "service");
    }

    const auto bus_result = open_system_bus();
    if (!bus_result) {
        return bus_result.error();
    }

    BusHandle handle{bus_result.value()};
    sd_bus* bus = handle.bus;

    // Create enable operation: symlink .service/wants/unit_name -> /etc/systemd/system/unit_name
    sd_bus_message* reply = nullptr;
    int r = sd_bus_call_method(bus,
                                "org.freedesktop.systemd1",
                                "/org/freedesktop/systemd1",
                                "org.freedesktop.systemd1.Manager",
                                "EnableUnitFiles",
                                nullptr,
                                &reply,
                                "as", 1, std::string(unit_name).c_str());
    if (r < 0) {
        if (reply) {
            sd_bus_message_unref(reply);
        }
        return Error::from_errno(-r, ErrorCode::IoError, "service", 
                                 "EnableUnitFiles call failed");
    }

    sd_bus_message_unref(reply);

    std::string msg = "Enabled service '" + std::string(unit_name) + "'";
    nizaw::core::Logger::instance().write(nizaw::core::LogLevel::Info, "service", msg);
    core::AuditLogger::instance().log("service", "enable", std::string(unit_name), true);
    return {};
}

Result<void> disable(std::string_view unit_name, const core::WriteOptions& options) {
    if (options.dry_run) {
        std::string msg = "Would disable service '" + std::string(unit_name) + "'";
        nizaw::core::Logger::instance().write(nizaw::core::LogLevel::Info, "service", msg);
        return {};
    }

    if (options.confirm_prompt) {
        nizaw::core::Logger::instance().write(nizaw::core::LogLevel::Warn, "service", "Confirmation required: " + *options.confirm_prompt);
        return Error(ErrorCode::ConfirmationRequired, 
                     "Confirmation required for disable", "service");
    }

    const auto bus_result = open_system_bus();
    if (!bus_result) {
        return bus_result.error();
    }

    BusHandle handle{bus_result.value()};
    sd_bus* bus = handle.bus;

    sd_bus_message* reply = nullptr;
    int r = sd_bus_call_method(bus,
                                "org.freedesktop.systemd1",
                                "/org/freedesktop/systemd1",
                                "org.freedesktop.systemd1.Manager",
                                "DisableUnitFiles",
                                nullptr,
                                &reply,
                                "as", 1, std::string(unit_name).c_str());
    if (r < 0) {
        if (reply) {
            sd_bus_message_unref(reply);
        }
        return Error::from_errno(-r, ErrorCode::IoError, "service", 
                                 "DisableUnitFiles call failed");
    }

    sd_bus_message_unref(reply);

    std::string msg = "Disabled service '" + std::string(unit_name) + "'";
    nizaw::core::Logger::instance().write(nizaw::core::LogLevel::Info, "service", msg);
    core::AuditLogger::instance().log("service", "disable", std::string(unit_name), true);
    return {};
}

}  // namespace nizaw::service
