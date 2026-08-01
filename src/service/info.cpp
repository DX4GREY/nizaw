#include "nizaw/service.hpp"

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

Result<uint64_t> get_main_pid(sd_bus* bus, const std::string& unit_path) {
    uint64_t pid = 0;
    int r = sd_bus_get_property_uint64(bus,
                                       "org.freedesktop.systemd1",
                                       unit_path.c_str(),
                                       "org.freedesktop.systemd1.Unit",
                                       "MainPID",
                                       &pid);
    if (r < 0) {
        return Error::from_errno(-r, ErrorCode::IoError, "service", "Failed to read MainPID property");
    }
    return pid;
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

}  // namespace nizaw::service
